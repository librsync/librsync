/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 1999, 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 1999 by Andrew Tridgell <tridge@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/** \file sumset.h
 * The rs_signature class implementation of a file signature. */
#ifndef SUMSET_H
#  define SUMSET_H

#  include <assert.h>
#  include <stddef.h>
#  include "hashtable.h"
#  include "checksum.h"
#  include "librsync.h"

/** Signature of a single block. */
typedef struct rs_block_sig {
    rs_weak_sum_t weak_sum;     /**< Block's weak checksum. */
    rs_strong_sum_t strong_sum; /**< Block's strong checksum. */
} rs_block_sig_t;

/** Signature of a whole file.
 *
 * This includes the all the block sums generated for a file and datastructures
 * for fast matching against them. */
struct rs_signature {
    int magic;                  /**< The signature magic value. */
    int block_len;              /**< The block length. */
    int strong_sum_len;         /**< The block strong sum length. */
    int count;                  /**< Total number of blocks. */
    int size;                   /**< Total number of blocks allocated. */
    void *block_sigs;           /**< The packed block_sigs for all blocks. */
    hashtable_t *hashtable;     /**< The hashtable for finding matches. */
    /* The is extra stats not included in the hashtable stats. */
#  ifndef HASHTABLE_NSTATS
    long calc_strong_count;     /**< The count of strongsum calcs done. */
#  endif
};

/** Initialize an rs_signature instance.
 *
 * \param *sig the signature to initialize.
 *
 * \param magic - the magic type to use (0 for "recommended").
 *
 * \param block_len - the block length to use (0 for "recommended").
 *
 * \param strong_len - the strongsum length to use (0 for "maximum", -1 for
 * "minimum"). Must be <= the max strongsum size for the strongsum type
 * indicated by the magic value.
 *
 * \param sig_fsize - the signature file size (-1 for "unknown"). Used to
 * preallocate required storage. */
rs_result rs_signature_init(rs_signature_t *sig, rs_magic_number magic,
                            size_t block_len, size_t strong_len,
                            rs_long_t sig_fsize);

/** Destroy an rs_signature instance. */
void rs_signature_done(rs_signature_t *sig);

/** Add a block to an rs_signature instance. */
rs_block_sig_t *rs_signature_add_block(rs_signature_t *sig,
                                       rs_weak_sum_t weak_sum,
                                       rs_strong_sum_t *strong_sum);

/** Find a matching block offset in a signature. */
rs_long_t rs_signature_find_match(rs_signature_t *sig, rs_weak_sum_t weak_sum,
                                  void const *buf, size_t len);

/** Assert that rs_sig_args() args for rs_signature_init() are valid.
 *
 * We don't use a static inline function here so that assert failure output
 * points at where rs_sig_args_check() was called from. */
#  define rs_sig_args_check(magic, block_len, strong_len) do {\
    assert(((magic) & ~0xff) == (RS_MD4_SIG_MAGIC & ~0xff));\
    assert(((magic) & 0xf0) == 0x30 || ((magic) & 0xf0) == 0x40);\
    assert((((magic) & 0x0f) == 0x06 &&\
	    (int)(strong_len) <= RS_MD4_SUM_LENGTH) ||\
	   (((magic) & 0x0f) == 0x07 &&\
	    (int)(strong_len) <= RS_BLAKE2_SUM_LENGTH));\
    assert(0 < (block_len));\
    assert(0 < (strong_len) && (strong_len) <= RS_MAX_STRONG_SUM_LENGTH);\
} while (0)

/** Assert that a signature is valid.
 *
 * We don't use a static inline function here so that assert failure output
 * points at where rs_signature_check() was called from. */
#  define rs_signature_check(sig) do {\
    rs_sig_args_check((sig)->magic, (sig)->block_len, (sig)->strong_sum_len);\
    assert(0 <= (sig)->count && (sig)->count <= (sig)->size);\
    assert(!(sig)->hashtable || (sig)->hashtable->count <= (sig)->count);\
} while (0)

/** Get the weaksum kind for a signature. */
static inline weaksum_kind_t rs_signature_weaksum_kind(rs_signature_t const
                                                       *sig)
{
    return (sig->magic & 0xf0) == 0x30 ? RS_ROLLSUM : RS_RABINKARP;
}

/** Get the strongsum kind for a signature. */
static inline strongsum_kind_t rs_signature_strongsum_kind(rs_signature_t const
                                                           *sig)
{
    return (sig->magic & 0x0f) == 0x06 ? RS_MD4 : RS_BLAKE2;
}

/** Calculate the weak sum of a buffer. */
static inline rs_weak_sum_t rs_signature_calc_weak_sum(rs_signature_t const
                                                       *sig, void const *buf,
                                                       size_t len)
{
    return rs_calc_weak_sum(rs_signature_weaksum_kind(sig), buf, len);
}

/** Calculate the strong sum of a buffer. */
static inline void rs_signature_calc_strong_sum(rs_signature_t const *sig,
                                                void const *buf, size_t len,
                                                rs_strong_sum_t *sum)
{
    rs_calc_strong_sum(rs_signature_strongsum_kind(sig), buf, len, sum);
}

#endif                          /* !SUMSET_H */
