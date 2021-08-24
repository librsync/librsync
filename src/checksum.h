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

/** \file checksum.h
 * Abstract wrappers around different weaksum and strongsum implementations. */
#ifndef CHECKSUM_H
#  define CHECKSUM_H

#  include <assert.h>
#  include <stddef.h>
#  include "librsync.h"
#  include "rollsum.h"
#  include "rabinkarp.h"
#  include "hashtable.h"

/** Weaksum implementations. */
typedef enum {
    RS_ROLLSUM,
    RS_RABINKARP,
} weaksum_kind_t;

/** Strongsum implementations. */
typedef enum {
    RS_MD4,
    RS_BLAKE2,
} strongsum_kind_t;

/** Abstract wrapper around weaksum implementations.
 *
 * This is a polymorphic interface to the different rollsum implementations.
 *
 * Historically rollsum methods were implemented as static inline functions
 * because they were small and needed to be fast. Now that we need to call
 * different methods for different rollsum implementations, they are getting
 * more complicated. Is it better to delegate calls to the right implementation
 * using static inline switch statements, or stop inlining them and use virtual
 * method pointers? Tests suggest inlined switch statements is faster. */
typedef struct weaksum {
    weaksum_kind_t kind;
    union {
        Rollsum rs;
        rabinkarp_t rk;
    } sum;
} weaksum_t;

static inline void weaksum_reset(weaksum_t *sum)
{
    if (sum->kind == RS_ROLLSUM)
        RollsumInit(&sum->sum.rs);
    else
        rabinkarp_init(&sum->sum.rk);
}

static inline void weaksum_init(weaksum_t *sum, weaksum_kind_t kind)
{
    assert(kind == RS_ROLLSUM || kind == RS_RABINKARP);
    sum->kind = kind;
    weaksum_reset(sum);
}

static inline size_t weaksum_count(weaksum_t *sum)
{
    /* We take advantage of sum->sum.rs.count overlaying sum->sum.rk.count. */
    return sum->sum.rs.count;
}

static inline void weaksum_update(weaksum_t *sum, const unsigned char *buf,
                                  size_t len)
{
    if (sum->kind == RS_ROLLSUM)
        RollsumUpdate(&sum->sum.rs, buf, len);
    else
        rabinkarp_update(&sum->sum.rk, buf, len);
}

static inline void weaksum_rotate(weaksum_t *sum, unsigned char out,
                                  unsigned char in)
{
    if (sum->kind == RS_ROLLSUM)
        RollsumRotate(&sum->sum.rs, out, in);
    else
        rabinkarp_rotate(&sum->sum.rk, out, in);
}

static inline void weaksum_rollin(weaksum_t *sum, unsigned char in)
{
    if (sum->kind == RS_ROLLSUM)
        RollsumRollin(&sum->sum.rs, in);
    else
        rabinkarp_rollin(&sum->sum.rk, in);
}

static inline void weaksum_rollout(weaksum_t *sum, unsigned char out)
{
    if (sum->kind == RS_ROLLSUM)
        RollsumRollout(&sum->sum.rs, out);
    else
        rabinkarp_rollout(&sum->sum.rk, out);
}

static inline rs_weak_sum_t weaksum_digest(weaksum_t *sum)
{
    if (sum->kind == RS_ROLLSUM)
        /* We apply mix32() to rollsums before using them for matching. */
        return mix32(RollsumDigest(&sum->sum.rs));
    else
        return rabinkarp_digest(&sum->sum.rk);
}

/** Calculate a weaksum.
 *
 * Note this does not apply mix32() to rollsum digests, unlike
 * weaksum_digest(). This is because rollsums are stored raw without mix32()
 * applied for backwards-compatibility, but we apply mix32() when adding them
 * into a signature and when getting the digest for calculating deltas. */
rs_weak_sum_t rs_calc_weak_sum(weaksum_kind_t kind, void const *buf,
                               size_t len);

/** Calculate a strongsum. */
void rs_calc_strong_sum(strongsum_kind_t kind, void const *buf, size_t len,
                        rs_strong_sum_t *sum);

#endif                          /* !CHECKSUM_H */
