#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
// kmalloc
#include <linux/slab.h>
// k_time
#include <linux/ktime.h>
// __copy_to_user
#include <asm/uaccess.h>
#include "bigNum_binary.h"
#include "stringAdd.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 92

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static ktime_t kt;

static long long fib_sequence_string(long long k, char *buf)
{
    /* FIXME: C99 variable-length array (VLA) is not allowed in Linux kernel.
     */
    str_t *f = kmalloc((k + 2) * sizeof(str_t), GFP_KERNEL);

    strncpy(f[0].numberStr, "0", 1);
    f[0].numberStr[1] = '\0';
    strncpy(f[1].numberStr, "1", 1);
    f[1].numberStr[1] = '\0';

    for (int i = 2; i <= k; i++) {
        add_str(f[i - 1].numberStr, f[i - 2].numberStr, f[i].numberStr);
    }
    size_t retSize = strlen(f[k].numberStr);
    reverse_str(f[k].numberStr, retSize);
    __copy_to_user(buf, f[k].numberStr, retSize);
    return retSize;
}

static long long fib_sequence(long long k, char *buf)
{
    bigNum_t *fib = kmalloc((k + 2) * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_init(&fib[0], 0);
    bigNum_init(&fib[1], 1);
    for (int i = 2; i <= k; i++) {
        // bigNum_init(&fib[i], 0);
        bigNum_add(&fib[i - 1], &fib[i - 2], &fib[i]);
    }

    __copy_to_user(buf, &fib[k], sizeof(bigNum_t));
    __copy_to_user(buf + 4, fib[k].digits, fib[k].len * sizeof(int));
    return 0;
}

static bigNum_t fib_helper(uint64_t n, bigNum_t *fib, bigNum_t *c)
{
    if (!n) {
        bigNum_init(&fib[n], 0);
        return fib[n];
    } else if (n <= 2) {
        bigNum_init(&fib[n], 1);
        return fib[n];
    } else if (fib[n].digits) {
        return fib[n];
    }
    bigNum_init(&fib[n], 0);
    bigNum_init(&c[0], 0);
    bigNum_init(&c[1], 0);
    uint64_t k = n / 2;
    bigNum_t a = fib_helper(k, fib, c);
    bigNum_t b = fib_helper(k + 1, fib, c);
    if (n % 2) {
        bigNum_square(&a, &c[0]);
        bigNum_square(&b, &c[1]);
        bigNum_add(&c[0], &c[1], &fib[n]);
    } else {
        bigNum_lshift(&b, &c[0]);
        bigNum_substract(&c[0], &a, &c[1]);
        bigNum_mul(&a, &c[1], &fib[n]);
    }
    return fib[n];
}

static long long fib_sequence_fast_doubling_recursive(long long k, char *buf)
{
    bigNum_t *c = kmalloc(2 * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_t *fib = kmalloc((k + 2) * sizeof(bigNum_t), GFP_KERNEL);
    fib_helper(k, fib, c);
    char *ret = bigNum_to_dec(&fib[k]);
    size_t retSize = strlen(ret);
    __copy_to_user(buf, ret, retSize);
    return retSize;
}

static long long fib_sequence_fast_doubling_iterative(long long k, char *buf)
{
    uint8_t bits = 0;
    for (uint32_t i = k; i; ++bits, i >>= 1)
        ;

    bigNum_t *res = kmalloc(4 * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_t *tmp = kmalloc(3 * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_init(&res[0], 0);
    bigNum_init(&res[1], 1);
    bigNum_init(&tmp[2], 0);
    for (uint32_t mask = 1 << (bits - 1); mask; mask >>= 1) {
        bigNum_lshift(&res[1], &tmp[0]);
        bigNum_substract(&tmp[0], &res[0], &tmp[1]);
        bigNum_mul(&res[0], &tmp[1], &res[2]);
        bigNum_square(&res[0], &tmp[0]);
        bigNum_square(&res[1], &tmp[1]);
        bigNum_add(&tmp[0], &tmp[1], &res[3]);
        if (mask & k) {
            bigNum_add(&tmp[2], &res[3], &res[0]);
            bigNum_add(&res[2], &res[3], &res[1]);
        } else {
            bigNum_add(&tmp[2], &res[2], &res[0]);
            bigNum_add(&tmp[2], &res[3], &res[1]);
        }
    }
    char *ret = bigNum_to_dec(&res[0]);
    size_t retSize = strlen(ret);
    __copy_to_user(buf, ret, retSize);
    return retSize;
}

static long long fib_sequence_fast_doubling_iterative_clz(long long k,
                                                          char *buf)
{
    uint8_t h = 63 - __builtin_clzll(k);

    bigNum_t *res = kmalloc(4 * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_t *tmp = kmalloc(3 * sizeof(bigNum_t), GFP_KERNEL);
    bigNum_init(&res[0], 0);
    bigNum_init(&res[1], 1);
    bigNum_init(&tmp[2], 0);
    for (uint32_t mask = 1 << h; mask; mask >>= 1) {
        bigNum_lshift(&res[1], &tmp[0]);
        bigNum_substract(&tmp[0], &res[0], &tmp[1]);
        bigNum_mul(&res[0], &tmp[1], &res[2]);
        bigNum_square(&res[0], &tmp[0]);
        bigNum_square(&res[1], &tmp[1]);
        bigNum_add(&tmp[0], &tmp[1], &res[3]);
        if (mask & k) {
            bigNum_add(&tmp[2], &res[3], &res[0]);
            bigNum_add(&res[2], &res[3], &res[1]);
        } else {
            bigNum_add(&tmp[2], &res[2], &res[0]);
            bigNum_add(&tmp[2], &res[3], &res[1]);
        }
    }
    char *ret = bigNum_to_dec(&res[0]);
    size_t retSize = strlen(ret);
    __copy_to_user(buf, ret, retSize);
    return retSize;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

static long long fib_time_proxy(long long k, char *buf, int flag)
{
    long long result = 0;
    kt = ktime_get();
    switch (flag) {
    case 0:
        result = fib_sequence(k, buf);
        break;
    case 1:
        result = fib_sequence_fast_doubling_recursive(k, buf);
        break;
    case 2:
        result = fib_sequence_fast_doubling_iterative(k, buf);
        break;
    case 3:
        result = fib_sequence_fast_doubling_iterative_clz(k, buf);
        break;
    case 4:
        result = fib_sequence_string(k, buf);
        break;
    }
    kt = ktime_sub(ktime_get(), kt);
    return result;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    return (ssize_t) fib_time_proxy(*offset, buf, 0);
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return ktime_to_ns(kt);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
