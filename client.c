#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define FIB_DEV "/dev/fibonacci"
#define THREAD_COUNT 100

int count = 0;
pthread_mutex_t fib_lock;

char *bigNum_to_dec(unsigned int *digits, int length)
{
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    size_t len = (8 * sizeof(int) * length -
                  __builtin_clz(digits[length - 1] ? digits[length - 1] : 1)) /
                     3.322 +
                 2;

    char *s = malloc(len);
    char *p = s;

    memset(s, '0', len - 1);
    s[len - 1] = '\0';

    for (int i = length - 1; i >= 0; i--) {
        for (unsigned int d = 0x80000000; d; d >>= 1) {
            /* binary -> decimal string */
            int carry = !!(d & digits[i]);
            for (int j = len - 2; j >= 0; j--) {
                int tmp = 2 * (s[j] - '0') + carry;  // double it
                s[j] = "0123456789"[tmp % 10];
                carry = tmp / 10;
                if (!s[j] && !carry)
                    break;
            }
        }
    }

    if (p[0] == '0' && p[1] != '\0')
        p++;

    memmove(s, p, strlen(p) + 1);
    return s;
}

// close the complier warning of unused parameter
void thread_func(int x __attribute_used__)
{
    unsigned int *read_buf = malloc(50 * sizeof(int));
    char write_buf[] = "";
    int offset = 500; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    write(fd, write_buf, 0);
    while (1) {
        pthread_mutex_lock(&fib_lock);
        ++count;
        if (count > 500)
            break;
        lseek(fd, count, SEEK_SET);
        int length = read(fd, read_buf, 1);
        printf("digits = %s\n", bigNum_to_dec(read_buf, length));
        pthread_mutex_unlock(&fib_lock);
    }
    pthread_mutex_unlock(&fib_lock);
    close(fd);
}

int main()
{
    pthread_t pt[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&pt[i], NULL, thread_func, NULL);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(pt[i], NULL);
    }

    return 0;
}
