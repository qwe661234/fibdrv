#include "bigNum.h"
#include <stdio.h>
#include <stdlib.h>
#define maxLen 8
#define base 1000000000U

bigNum_t *bigNum_init(int num)
{
    bigNum_t *ret = malloc(sizeof(bigNum_t));
    ret->digits = malloc(maxLen * sizeof(uint32_t));
    ret->len = 1;
    if (num < 0) {
        ret->sign = 1;
        num *= -1;
    }
    if (num > base)
        ret->digits[1] = num / base;
    ret->digits[0] = num % base;
    return ret;
};
int main()
{
    bigNum_t *n = bigNum_init(INT32_MAX);
    printf("%u\n", n->digits[1]);
    printf("%u\n", n->digits[0]);
}

// void bigNum_to_dec(bigNum_t *n) {

// };
// void bigNum_add(bigNum_t *a, bigNum_t *b, bigNum_t *c);