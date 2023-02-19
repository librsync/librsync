/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * rabinkarp -- The RabinKarp rolling checksum.
 *
 * Copyright (C) 2019 by Donovan Baarda <abo@minkirri.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"             /* IWYU pragma: keep */
#include "rabinkarp.h"

/* Constant for RABINKARP_MULT^2. */
#define RABINKARP_MULT2 0xa5b71959U

/* Macros for doing 16 bytes with 2 mults that can be done in parallel. Testing
   showed this as a performance sweet spot vs 16x1, 8x2, 4x4 1x16 alternative
   arrangements. */
#define PAR2X1(hash,buf,i) (RABINKARP_MULT2*(hash) + \
			    RABINKARP_MULT*buf[i] + \
			    buf[i+1])
#define PAR2X2(hash,buf,i) PAR2X1(PAR2X1(hash,buf,i),buf,i+2)
#define PAR2X4(hash,buf,i) PAR2X2(PAR2X2(hash,buf,i),buf,i+4)
#define PAR2X8(hash,buf) PAR2X4(PAR2X4(hash,buf,0),buf,8)

/* Table of RABINKARP_MULT^(2^(i+1)) for power lookups. */
const static uint32_t RABINKARP_MULT_POW2[32] = {
    0x08104225U,
    0xa5b71959U,
    0xf9c080f1U,
    0x7c71e2e1U,
    0x0bb409c1U,
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
    0x03800001U,
    0x07000001U,
    0x0e000001U,
    0x1c000001U,
    0x38000001U,
    0x70000001U,
    0xe0000001U,
    0xc0000001U,
    0x80000001U,
    0x00000001U,
    0x00000001U
};

/* Get the value of RABINKARP_MULT^n. */
static inline uint32_t rabinkarp_pow(uint32_t n)
{
    const uint32_t *m = RABINKARP_MULT_POW2;
    uint32_t ans = 1;
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
    uint32_t hash = sum->hash;

    while (n >= 16) {
        hash = PAR2X8(hash, buf);
        buf += 16;
        n -= 16;
    }
    while (n) {
        hash = RABINKARP_MULT * hash + *buf++;
        n--;
    }
    sum->hash = hash;
    sum->count += len;
    sum->mult *= rabinkarp_pow((uint32_t)len);
}
