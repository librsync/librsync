#include "rabinkarp.h"

#define MULT1(hash,buf,i) (hash) * RABINKARP_MULT + buf[i]
#define MULT2(hash,buf,i) MULT1(MULT1(hash,buf,i),buf,i+1)
#define MULT4(hash,buf,i) MULT2(MULT2(hash,buf,i),buf,i+2)
#define MULT8(hash,buf,i) MULT4(MULT4(hash,buf,i),buf,i+4)
#define MULT16(hash,buf) MULT8(MULT8(hash,buf,0),buf,8)

#define DO1(hash,buf,i) {hash = RABINKARP_MULT * hash + buf[i];}
#define DO2(hash,buf,i)  DO1(hash,buf,i); DO1(hash,buf,i+1);
#define DO4(hash,buf,i)  DO2(hash,buf,i); DO2(hash,buf,i+2);
#define DO8(hash,buf,i)  DO4(hash,buf,i); DO4(hash,buf,i+4);
#define DO16(hash,buf)   DO8(hash,buf,0); DO8(hash,buf,8);

static inline uint32_t uint32_pow(uint32_t m, size_t p)
{
    uint32_t ans = 1;
    while (p) {
        if (p & 1) {
            ans *= m;
        }
        m *= m;
        p >>= 1;
    }
    return ans;
}

void rabinkarp_update(rabinkarp_t *sum, const unsigned char *buf, size_t len)
{
    size_t n = len;
    uint32_t hash = sum->hash;

    while (n >= 16) {
        hash = MULT16(hash, buf);
        // DO16(sum->hash,buf);
        buf += 16;
        n -= 16;
    }
    while (n) {
        hash = hash * RABINKARP_MULT + *buf++;
        n--;
    }
    sum->hash = hash;
    sum->count += len;
    sum->mult *= uint32_pow(RABINKARP_MULT, len);
}
