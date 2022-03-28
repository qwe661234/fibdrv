#include <linux/slab.h>
#define maxLen 100
#define base 1000000000U

typedef struct bigNum {
    uint8_t len;
    uint32_t *digits;
} bigNum_t;

// void bigNum_init(bigNum_t *n, int32_t num);
// void bigNum_to_dec(bigNum_t *n);
// void bigNum_add(bigNum_t *a, bigNum_t *b, bigNum_t *c);
// void bigNum_substract(bigNum_t *a, bigNum_t *b, bigNum_t *c);
// void bigNum_mul(bigNum_t *a, bigNum_t *b, bigNum_t *c);
// // void bigNum_rshift(bigNum_t *a);
// void bigNum_mul_lshift(bigNum_t *a, bigNum_t *b);
// #include "bigNum.h"


void bigNum_init(bigNum_t *n, int num)
{
    n->digits = kmalloc(maxLen * sizeof(uint32_t), GFP_KERNEL);
    n->len = 1;
    if (num >= base) {
        n->digits[1] = num / base;
        n->len++;
    }
    n->digits[0] = num % base;
};

void bigNum_to_dec(bigNum_t *n)
{
    printk("%u", n->digits[n->len - 1]);
    for (int i = n->len - 2; i >= 0; i--) {
        printk("%09u", n->digits[i]);
    }
    printk("\n");
};

void bigNum_add(bigNum_t *a, bigNum_t *b, bigNum_t *c)
{
    uint8_t carry = 0;
    uint64_t sum = 0;
    if (a->len >= b->len) {
        c->len = a->len;
        for (int i = 0; i < b->len; i++) {
            sum = a->digits[i] + b->digits[i] + carry;
            carry = sum / base;
            c->digits[i] = sum % base;
        }
        for (int i = b->len; i < a->len; i++) {
            sum = a->digits[i] + carry;
            carry = sum / base;
            c->digits[i] = sum % base;
        }
    } else {
        c->len = b->len;
        for (int i = 0; i < a->len; i++) {
            sum = a->digits[i] + b->digits[i] + carry;
            carry = sum / base;
            c->digits[i] = sum % base;
        }
        for (int i = a->len; i < b->len; i++) {
            sum = b->digits[i] + carry;
            carry = sum / base;
            c->digits[i] = sum % base;
        }
    }
    if (carry) {
        c->digits[c->len] = carry;
        c->len++;
    }
};

// void bigNum_substract(bigNum_t *a, bigNum_t *b, bigNum_t *c)
// {
//     c->len = a->len;
//     for (int i = 0; i < a->len; i++) {
//         if (a->digits[i] < b->digits[i]) {
//             a->digits[i + 1]--;
//             a->digits[i] += base;
//         }
//         c->digits[i] = a->digits[i] - b->digits[i];
//     }
//     if (!a->digits[a->len - 1])
//         c->len--;
// };

// void bigNum_mul(bigNum_t *a, bigNum_t *b, bigNum_t *c)
// {
//     uint64_t carry = 0, sum = 0;
//     c->len = b->len + a->len;
//     for (int i = 0; i < b->len; i++) {
//         for (int j = 0; j < a->len; j++) {
//             sum = (uint64_t) b->digits[i] * a->digits[j] + carry +
//                   c->digits[j + i];
//             carry = sum / base;
//             c->digits[j + i] = sum % base;
//         }
//         if (carry) {
//             c->digits[i + a->len] = carry;
//             carry = 0;
//         }
//     }
//     if (!c->digits[c->len - 1])
//         c->len--;
// }

// void bigNum_rshift(bigNum_t *a)
// {
//     uint8_t carry = 0, tmp;
//     for (int i = a->len - 1; i >= 0; i--) {
//         tmp = a->digits[i] & 1;
//         a->digits[i] >>= 1;
//         if (carry)
//             a->digits[i] += base >> 1;
//         carry = tmp;
//     }
//     if (!a->digits[a->len - 1] && a->len > 1)
//         a->len--;
// };

// void bigNum_lshift(bigNum_t *a, bigNum_t *b)
// {
//     uint8_t carry = 0;
//     uint32_t sum = 0;
//     b->len = a->len;
//     for (int i = 0; i < b->len; i++) {
//         sum = a->digits[i] * 2 + carry;
//         carry = sum / base;
//         b->digits[i] = sum % base;
//     }
//     if (carry) {
//         b->digits[b->len] = carry;
//         b->len++;
//     }
// };