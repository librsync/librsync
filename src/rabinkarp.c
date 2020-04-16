#include "rabinkarp.h"

/* Constants for RABINKARP_MULT^i. */
#define RABINKARP_MULT1 RABINKARP_MULT
#define RABINKARP_MULT2 (RABINKARP_MULT1*RABINKARP_MULT1)
#define RABINKARP_MULT3 (RABINKARP_MULT1*RABINKARP_MULT2)
#define RABINKARP_MULT4 (RABINKARP_MULT1*RABINKARP_MULT3)

/* Macro for doing 4 bytes in parallel. */
#define DOMULT4(hash, buf) (RABINKARP_MULT4*(hash) + \
			    RABINKARP_MULT3*buf[0] + \
			    RABINKARP_MULT2*buf[1] + \
			    RABINKARP_MULT1*buf[2] + buf[3])

/* Table of RABINKARP_MULT^(2^(i+1)) for power lookups. */
const static uint32_t RABINKARP_MULT_POW2[32] = {
    0x8104225U,
    0xa5b71959U,
    0xf9c080f1U,
    0x7c71e2e1U,
    0xbb409c1U,
    0x4dc72381U,
    0xd17a8701U,
    0x96260e01U,
    0x55101c01U,
    0x2d303801U,
    0x66a07001U,
    0xfe40e001U,
    0xc081c001U,
    0x91038001U,
    0x62070001U,
    0xc40e0001U,
    0x881c0001U,
    0x10380001U,
    0x20700001U,
    0x40e00001U,
    0x81c00001U,
    0x3800001U,
    0x7000001U,
    0xe000001U,
    0x1c000001U,
    0x38000001U,
    0x70000001U,
    0xe0000001U,
    0xc0000001U,
    0x80000001U,
    0x1U,
    0x1U
};

/* Get the value of RABINKARP_MULT^p. */
static inline uint32_t rabinkarp_pow(size_t p) {
    /* Truncate p to 32 bits since higher bits don't affect result. */
    uint32_t n = p;
    uint32_t ans = 1;
    const uint32_t *m = RABINKARP_MULT_POW2;
    while (n) {
        if (n & 1) {
            ans *= *m;
        }
        m++;
        n >>= 1;
    }
    return ans;
}

void rabinkarp_update(rabinkarp_t *sum, const unsigned char *buf, size_t len)
{
    size_t n = len;
    uint_fast32_t hash = sum->hash;

    while (n >= 4) {
        // hash = MULT16(hash, buf);
        // DO16(sum->hash,buf);
        hash = DOMULT4(hash, buf);
        buf += 4;
        n -= 4;
    }
    while (n) {
        hash = RABINKARP_MULT * hash + *buf++;
        n--;
    }
    sum->hash = hash;
    sum->count += len;
    sum->mult *= rabinkarp_pow(len);
}
