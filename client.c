#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#define FIB_DEV "/dev/fibonacci"

static inline long long get_nanotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

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

int main()
{
    unsigned int *read_buf = malloc(50 * sizeof(int));
    // char read_buf[100] = "";
    char write_buf[] = "";
    int offset = 100; /* TODO: try test something bigger than the limit */

    int fd = open(FIB_DEV, O_RDWR);
    FILE *data = fopen("data.txt", "w");

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    write(fd, write_buf, 0);
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        long long start = get_nanotime();
        int length = read(fd, read_buf, 1);
        long long utime = get_nanotime() - start;
        long long ktime = write(fd, write_buf, 0);
        fprintf(data, "%d %lld %lld %lld\n", i, ktime, utime, utime - ktime);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%d.\n",
               i, length);
        printf("digits = %s\n", bigNum_to_dec(read_buf, sz));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", ktime);
    }

    close(fd);
    fclose(data);
    return 0;
}
