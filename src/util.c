/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
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

                                /*=
                                 | On heroin, I have all the answers.
                                 */

#include "config.h"             /* IWYU pragma: keep */
#include <stdlib.h>
#include <string.h>
#include "librsync.h"
#include "util.h"
#include "trace.h"

void rs_bzero(void *buf, size_t size)
{
    memset(buf, 0, size);
}

void *rs_alloc_struct0(size_t size, char const *name)
{
    void *p;

    if (!(p = malloc(size))) {
        rs_fatal("couldn't allocate instance of %s", name);
    }
    rs_bzero(p, size);
    return p;
}

void *rs_alloc(size_t size, char const *name)
{
    void *p;

    if (!(p = malloc(size))) {
        rs_fatal("couldn't allocate instance of %s", name);
    }

    return p;
}

void *rs_realloc(void *ptr, size_t size, char const *name)
{
    void *p;

    if (!(p = realloc(ptr, size))) {
        rs_fatal("couldn't reallocate instance of %s", name);
    }
    return p;
}

int rs_long_ln2(rs_long_t v)
{
    int n;

    /* Count the number of shifts to zero v. */
    for (n = 0; (v >>= 1); n++) ;
    return n;
}

int rs_long_sqrt(rs_long_t v)
{
    rs_long_t n, b;

    /* Find the most significant bit of the root. */
    for (b = 1, n = v; (n >>= 2); b <<= 1) ;
    /* Walk down the bits of the root. */
    for (n = 0; b; b >>= 1) {
        /* Set the bit in the answer n. */
        n |= b;
        /* If n^2 is too big, clear the bit. */
        if (n * n > v)
            n ^= b;
    }
    return (int)n;
}
