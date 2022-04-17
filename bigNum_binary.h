#include <linux/slab.h>

/* digits[len- 1] = msb, digits[0] = lsb */
typedef struct bigNum {
    uint8_t len;
    uint32_t *digits;
} bigNum_t;

/*
 * output bn to decimal string
 * Note: the returned string should be freed with kfree()
 */
void bigNum_init(bigNum_t *n, uint32_t num)
{
    n->digits = kmalloc(sizeof(uint32_t), GFP_KERNEL);
    n->len = 1;
    n->digits[0] = num;
}

char *bigNum_to_dec(bigNum_t *n)
{
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    size_t len = (8 * sizeof(int) * n->len) / 3 + 2;
    char *s = kmalloc(len, GFP_KERNEL);
    char *p = s;

    memset(s, '0', len - 1);
    s[len - 1] = '\0';

    for (int i = n->len - 1; i >= 0; i--) {
        for (unsigned int d = 1U << 31; d; d >>= 1) {
            /* binary -> decimal string */
            int carry = !!(d & n->digits[i]);
            for (int j = len - 2; j >= 0; j--) {
                s[j] += s[j] - '0' + carry;  // double it
                carry = (s[j] > '9');
                if (carry)
                    s[j] -= 10;
            }
        }
    }
    // skip leading zero
    while (p[0] == '0' && p[1] != '\0') {
        p++;
    }
    memmove(s, p, strlen(p) + 1);
    return s;
}

void bigNum_add(bigNum_t *a, bigNum_t *b, bigNum_t *c)
{
    if (c->digits)
        kfree(c->digits);
    uint8_t carry = 0;
    uint64_t sum;
    if (a->len >= b->len) {
        c->digits = kmalloc((a->len + 1) * sizeof(int), GFP_KERNEL);
        c->len = a->len;
        for (int i = 0; i < b->len; i++) {
            sum = (uint64_t) a->digits[i] + b->digits[i] + carry;
            c->digits[i] = sum & __UINT32_MAX__;
            carry = sum >> 32;
        }
        for (int i = b->len; i < a->len; i++) {
            sum = (uint64_t) a->digits[i] + carry;
            c->digits[i] = sum & __UINT32_MAX__;
            carry = sum >> 32;
        }
    } else {
        c->digits = kmalloc((b->len + 1) * sizeof(int), GFP_KERNEL);
        c->len = b->len;
        for (int i = 0; i < a->len; i++) {
            sum = (uint64_t) a->digits[i] + b->digits[i] + carry;
            c->digits[i] = sum & __UINT32_MAX__;
            carry = sum >> 32;
        }
        for (int i = a->len; i < b->len; i++) {
            sum = (uint64_t) b->digits[i] + carry;
            c->digits[i] = sum & __UINT32_MAX__;
            carry = sum >> 32;
        }
    }
    if (carry) {
        c->digits[c->len] = carry;
        c->len++;
    }
};

void bigNum_mul(bigNum_t *a, bigNum_t *b, bigNum_t *c)
{
    if (c->digits)
        kfree(c->digits);
    uint64_t carry = 0, sum, tmp;
    int num = (a->len << 5) + (b->len << 5) -
              __builtin_clz(a->digits[a->len - 1]) -
              __builtin_clz(b->digits[b->len - 1]);
    num = !!(num & 31) + (num >> 5);
    c->digits = kmalloc(num * sizeof(int), GFP_KERNEL);
    c->len = num;
    for (int i = 0; i < a->len; i++) {
        for (int j = 0; j < b->len; j++) {
            tmp = carry;
            sum = (uint64_t) a->digits[i] * b->digits[j];
            carry = sum >> 32;
            sum = (sum & 0xffffffff) + tmp + c->digits[i + j];
            carry += sum >> 32;
            c->digits[i + j] = sum & 0xffffffff;
        }
        if (carry) {
            c->digits[i + b->len] = carry;
            carry = 0;
        }
    }
    if (carry) {
        c->digits[num - 1] = carry;
    }
};

void bigNum_square(bigNum_t *a, bigNum_t *b)
{
    if (b->digits)
        kfree(b->digits);
    uint64_t carry = 0, sum, tmp;
    int num = (a->len << 5) - __builtin_clz(a->digits[a->len - 1]);
    num <<= 1;
    num = !!(num & 31) + (num >> 5);
    b->digits = kmalloc(num * sizeof(int), GFP_KERNEL);
    b->len = num;
    for (int i = 0; i < a->len; i++) {
        for (int j = 0; j < a->len; j++) {
            tmp = carry;
            sum = (uint64_t) a->digits[i] * a->digits[j];
            carry = sum >> 32;
            sum = (sum & 0xffffffff) + tmp + b->digits[i + j];
            carry += sum >> 32;
            b->digits[i + j] = sum & 0xffffffff;
        }
        if (carry) {
            b->digits[i + a->len] = carry;
            carry = 0;
        }
    }
    if (carry) {
        b->digits[num - 1] = carry;
    }
};

void bigNum_substract(bigNum_t *a, bigNum_t *b, bigNum_t *c)
{
    if (c->digits)
        kfree(c->digits);
    c->len = a->len;
    c->digits = kmalloc(c->len * sizeof(int), GFP_KERNEL);
    for (int i = 0; i < a->len; i++) {
        if (a->digits[i] < b->digits[i]) {
            a->digits[i + 1]--;
            c->digits[i] = __UINT32_MAX__ - (b->digits[i] - a->digits[i]) + 1;
        }
        c->digits[i] = a->digits[i] - b->digits[i];
    }
    if (!a->digits[a->len - 1])
        c->len--;
};

void bigNum_lshift(bigNum_t *a, bigNum_t *b)
{
    if (b->digits)
        kfree(b->digits);
    b->len = a->len + !!(__builtin_clz(a->digits[a->len - 1]) == 0);
    b->digits = kmalloc(b->len * sizeof(int), GFP_KERNEL);
    uint8_t carry = 0;

    for (int i = 0; i < a->len; i++) {
        uint64_t sum = ((uint64_t) a->digits[i] << 1) + carry;
        carry = sum >> 32;
        b->digits[i] = sum & 0xffffffff;
    }

    if (carry) {
        b->digits[b->len - 1] = carry;
    }
};

// static bigNum_t *fib_sequence(long long k)
// {
//     bigNum_t *fib = malloc((k + 2) * sizeof(bigNum_t));
//     bigNum_init(&fib[0], 0);
//     bigNum_init(&fib[1], 1);
//     for (int i = 2; i <= k; i++) {
//         bigNum_add(&fib[i - 1], &fib[i - 2], &fib[i]);
//     }
//     return &fib[k];
// }

// static bigNum_t fib_helper(uint64_t n, bigNum_t *fib, bigNum_t *c)
// {
//     if (!n) {
//         bigNum_init(&fib[n], 0);
//         return fib[n];
//     } else if (n <= 2) {
//         bigNum_init(&fib[n], 1);
//         return fib[n];
//     } else if (fib[n].digits) {
//         return fib[n];
//     }

//     uint64_t k = n / 2;
//     bigNum_t a = fib_helper(k, fib, c);
//     bigNum_t b = fib_helper(k + 1, fib, c);
//     if (n % 2) {
//         bigNum_square(&a, &c[0]);
//         bigNum_square(&b, &c[1]);
//         bigNum_add(&c[0], &c[1], &fib[n]);
//     } else {
//         bigNum_lshift(&b, &c[0]);
//         bigNum_substract(&c[0], &a, &c[1]);
//         bigNum_mul(&a, &c[1], &fib[n]);
//     }
//     return fib[n];
// }

// static bigNum_t *fib_sequence_fast_doubling_recursive(long long k)
// {
//     bigNum_t *c = malloc(2 * sizeof(bigNum_t));
//     bigNum_t *fib = malloc((k + 2) * sizeof(bigNum_t));
//     fib_helper(k, fib, c);
//     return &fib[k];
// }

// static bigNum_t *fib_sequence_fast_doubling_iterative(long long k)
// {
//     uint8_t h = 63 - __builtin_clzll(k);

//     bigNum_t *res = malloc(4 * sizeof(bigNum_t));
//     bigNum_t *tmp = malloc(3 * sizeof(bigNum_t));
//     bigNum_init(&res[0], 0);
//     bigNum_init(&res[1], 1);
//     bigNum_init(&tmp[2], 0);
//     for (uint32_t mask = 1 << h; mask; mask >>= 1) {
//         bigNum_lshift(&res[1], &tmp[0]);
//         bigNum_substract(&tmp[0], &res[0], &tmp[1]);
//         bigNum_mul(&res[0], &tmp[1], &res[2]);
//         bigNum_square(&res[0], &tmp[0]);
//         bigNum_square(&res[1], &tmp[1]);
//         bigNum_add(&tmp[0], &tmp[1], &res[3]);
//         if (mask & k) {
//             bigNum_add(&tmp[2], &res[3], &res[0]);
//             bigNum_add(&res[2], &res[3], &res[1]);
//         } else {
//             bigNum_add(&tmp[2], &res[2], &res[0]);
//             bigNum_add(&tmp[2], &res[3], &res[1]);
//         }
//         printf("%s\n", bigNum_to_dec(&res[0]));
//         printf("%s\n", bigNum_to_dec(&res[1]));
//     }
//     return &res[0];
// }