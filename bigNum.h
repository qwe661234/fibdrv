#include <stdint.h>

typedef struct bigNum {
    uint8_t len;
    uint8_t sign;
    uint32_t *digits;
} bigNum_t;

bigNum_t *bigNum_init(int32_t num);
void bigNum_to_dec(bigNum_t *n);
void bigNum_add(bigNum_t *a, bigNum_t *b, bigNum_t *c);
