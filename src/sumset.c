/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
 *
 * Copyright (C) 1999, 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 1999 by Andrew Tridgell
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
#include <stdlib.h>
#include <string.h>
#include "librsync.h"
#include "sumset.h"
#include "trace.h"
#include "util.h"

static void rs_block_sig_init(rs_block_sig_t *sig, rs_weak_sum_t weak_sum,
                              rs_strong_sum_t *strong_sum, int strong_len)
{
    sig->weak_sum = weak_sum;
    if (strong_sum)
        memcpy(sig->strong_sum, strong_sum, (size_t)strong_len);
}

static inline unsigned rs_block_sig_hash(const rs_block_sig_t *sig)
{
    return (unsigned)sig->weak_sum;
}

typedef struct rs_block_match {
    rs_block_sig_t block_sig;
    rs_signature_t *signature;
    const void *buf;
    size_t len;
} rs_block_match_t;

static void rs_block_match_init(rs_block_match_t *match, rs_signature_t *sig,
                                rs_weak_sum_t weak_sum,
                                rs_strong_sum_t *strong_sum, const void *buf,
                                size_t len)
{
    rs_block_sig_init(&match->block_sig, weak_sum, strong_sum,
                      sig->strong_sum_len);
    match->signature = sig;
    match->buf = buf;
    match->len = len;
}

static inline int rs_block_match_cmp(rs_block_match_t *match,
                                     const rs_block_sig_t *block_sig)
{
    /* If buf is not NULL, the strong sum is yet to be calculated. */
    if (match->buf) {
#ifndef HASHTABLE_NSTATS
        match->signature->calc_strong_count++;
#endif
        rs_signature_calc_strong_sum(match->signature, match->buf, match->len,
                                     &(match->block_sig.strong_sum));
        match->buf = NULL;
    }
    return memcmp(&match->block_sig.strong_sum, &block_sig->strong_sum,
                  (size_t)match->signature->strong_sum_len);
}

/* Disable mix32() in the hashtable because RabinKarp doesn't need it. We
   manually apply mix32() to rollsums before using them in the hashtable. */
#define HASHTABLE_NMIX32
/* Instantiate hashtable for rs_block_sig and rs_block_match. */
#define ENTRY rs_block_sig
#define MATCH rs_block_match
#define NAME hashtable
#include "hashtable.h"

/* Get the size of a packed rs_block_sig_t. */
static inline size_t rs_block_sig_size(const rs_signature_t *sig)
{
    /* Round up to multiple of sizeof(weak_sum) to align memory correctly. */
    const size_t mask = sizeof(rs_weak_sum_t)- 1;
    return (offsetof(rs_block_sig_t, strong_sum) +
            (((size_t)sig->strong_sum_len + mask) & ~mask));
}

/* Get the pointer to the block_sig_t from a block index. */
static inline rs_block_sig_t *rs_block_sig_ptr(const rs_signature_t *sig,
                                               int block_idx)
{
    return (rs_block_sig_t *)((char *)sig->block_sigs +
                               block_idx * rs_block_sig_size(sig));
}

/* Get the index of a block from a block_sig_t pointer. */
static inline int rs_block_sig_idx(const rs_signature_t *sig,
                                   rs_block_sig_t *block_sig)
{
    return (int)(((char *)block_sig -
                  (char *)sig->block_sigs) / rs_block_sig_size(sig));
}

rs_result rs_sig_args(rs_long_t old_fsize, rs_magic_number * magic,
                      size_t *block_len, size_t *strong_len)
{
    size_t rec_block_len;       /* the recomended block_len for the given
                                   old_fsize. */
    size_t min_strong_len;      /* the minimum strong_len for the given
                                   old_fsize and block_len. */
    size_t max_strong_len;      /* the maximum strong_len for the given magic. */

    /* Check and set default arguments. */
    *magic = *magic ? *magic : RS_RK_BLAKE2_SIG_MAGIC;
    switch (*magic) {
    case RS_BLAKE2_SIG_MAGIC:
    case RS_RK_BLAKE2_SIG_MAGIC:
        max_strong_len = RS_BLAKE2_SUM_LENGTH;
        break;
    case RS_MD4_SIG_MAGIC:
    case RS_RK_MD4_SIG_MAGIC:
        max_strong_len = RS_MD4_SUM_LENGTH;
        break;
    default:
        rs_error("invalid magic %#x", *magic);
        return RS_BAD_MAGIC;
    }
    /* The recommended block_len is sqrt(old_fsize) with a 256 min size rounded
       down to a multiple of the 128 byte blake2b blocksize to give a
       reasonable compromise between signature size, delta size, and
       performance. If the old_fsize is unknown, we use the default. */
    if (old_fsize < 0) {
        rec_block_len = RS_DEFAULT_BLOCK_LEN;
    } else {
        rec_block_len =
            old_fsize <= 256 * 256 ? 256 : rs_long_sqrt(old_fsize) & ~127;
    }
    if (*block_len == 0)
        *block_len = rec_block_len;
    /* The recommended strong_len assumes the worst case new_fsize = old_fsize
       + 16MB with no matches. This results in comparing a block at every byte
       offset against all the blocks in the signature, or new_fsize*block_num
       comparisons. With N bits in the blocksig, there is a 1/2^N chance per
       comparison of a hash colision. So with 2^N attempts there would be a
       fair chance of having a collision. So we want to round up to the next
       byte, add an extra 2 bytes (16 bits) in the strongsum, and assume the
       weaksum is worth another 16 bits, for at least 32 bits extra, giving a
       worst case 1/2^32 chance of having a hash collision per delta. If
       old_fsize is unknown we use a conservative default. */
    if (old_fsize < 0) {
        min_strong_len = RS_DEFAULT_MIN_STRONG_LEN;
    } else {
        min_strong_len =
            2 + (rs_long_ln2(old_fsize + ((rs_long_t)1 << 24)) +
                 rs_long_ln2(old_fsize / *block_len + 1) + 7) / 8;
    }
    if (*strong_len == 0)
        *strong_len = max_strong_len;
    else if (*strong_len == -1)
        *strong_len = min_strong_len;
    else if (old_fsize >= 0 && *strong_len < min_strong_len) {
        rs_warn("strong_len=" FMT_SIZE " smaller than recommended minimum "
                FMT_SIZE " for old_fsize=" FMT_LONG " with block_len=" FMT_SIZE,
                *strong_len, min_strong_len, old_fsize, *block_len);
    } else if (*strong_len > max_strong_len) {
        rs_error("invalid strong_len=" FMT_SIZE " for magic=%#x", *strong_len,
                 (int)*magic);
        return RS_PARAM_ERROR;
    }
    rs_sig_args_check(*magic, *block_len, *strong_len);
    return RS_DONE;
}

rs_result rs_signature_init(rs_signature_t *sig, rs_magic_number magic,
                            size_t block_len, size_t strong_len,
                            rs_long_t sig_fsize)
{
    rs_result result;

    /* Check and set default arguments, using old_fsize=-1 for unknown. */
    if ((result = rs_sig_args(-1, &magic, &block_len, &strong_len)) != RS_DONE)
        return result;
    /* Set attributes from args. */
    sig->magic = magic;
    sig->block_len = (int)block_len;
    sig->strong_sum_len = (int)strong_len;
    sig->count = 0;
    /* Calculate the number of blocks if we have the signature file size. */
    /* Magic+header is 12 bytes, each block thereafter is 4 bytes
       weak_sum+strong_sum_len bytes */
    sig->size = (int)(sig_fsize < 12 ? 0 : (sig_fsize - 12) / (4 + strong_len));
    if (sig->size)
        sig->block_sigs =
            rs_alloc(sig->size * rs_block_sig_size(sig),
                     "signature->block_sigs");
    else
        sig->block_sigs = NULL;
    sig->hashtable = NULL;
#ifndef HASHTABLE_NSTATS
    sig->calc_strong_count = 0;
#endif
    rs_signature_check(sig);
    return RS_DONE;
}

void rs_signature_done(rs_signature_t *sig)
{
    hashtable_free(sig->hashtable);
    free(sig->block_sigs);
    rs_bzero(sig, sizeof(*sig));
}

rs_block_sig_t *rs_signature_add_block(rs_signature_t *sig,
                                       rs_weak_sum_t weak_sum,
                                       rs_strong_sum_t *strong_sum)
{
    rs_signature_check(sig);
    /* Apply mix32() to rollsum weaksums to improve their distribution. */
    if (rs_signature_weaksum_kind(sig) == RS_ROLLSUM)
        weak_sum = mix32(weak_sum);
    /* If block_sigs is full, allocate more space. */
    if (sig->count == sig->size) {
        sig->size = sig->size ? sig->size * 2 : 16;
        sig->block_sigs =
            rs_realloc(sig->block_sigs, sig->size * rs_block_sig_size(sig),
                       "signature->block_sigs");
    }
    rs_block_sig_t *b = rs_block_sig_ptr(sig, sig->count++);
    rs_block_sig_init(b, weak_sum, strong_sum, sig->strong_sum_len);
    return b;
}

rs_long_t rs_signature_find_match(rs_signature_t *sig, rs_weak_sum_t weak_sum,
                                  void const *buf, size_t len)
{
    rs_block_match_t m;
    rs_block_sig_t *b;

    rs_signature_check(sig);
    rs_block_match_init(&m, sig, weak_sum, NULL, buf, len);
    if ((b = hashtable_find(sig->hashtable, &m))) {
        return (rs_long_t)rs_block_sig_idx(sig, b) * sig->block_len;
    }
    return -1;
}

void rs_signature_log_stats(rs_signature_t const *sig)
{
#ifndef HASHTABLE_NSTATS
    hashtable_t *t = sig->hashtable;

    rs_log(RS_LOG_INFO | RS_LOG_NONAME,
           "match statistics: signature[%ld searches, %ld (%.3f%%) matches, "
           "%ld (%.3fx) weak sum compares, %ld (%.3f%%) strong sum compares, "
           "%ld (%.3f%%) strong sum calcs]", t->find_count, t->match_count,
           100.0 * (double)t->match_count / (double)t->find_count,
           t->hashcmp_count, (double)t->hashcmp_count / (double)t->find_count,
           t->entrycmp_count,
           100.0 * (double)t->entrycmp_count / (double)t->find_count,
           sig->calc_strong_count,
           100.0 * (double)sig->calc_strong_count / (double)t->find_count);
#endif
}

rs_result rs_build_hash_table(rs_signature_t *sig)
{
    rs_block_match_t m;
    rs_block_sig_t *b;
    int i;

    rs_signature_check(sig);
    sig->hashtable = hashtable_new(sig->count);
    if (!sig->hashtable)
        return RS_MEM_ERROR;
    for (i = 0; i < sig->count; i++) {
        b = rs_block_sig_ptr(sig, i);
        rs_block_match_init(&m, sig, b->weak_sum, &b->strong_sum, NULL, 0);
        if (!hashtable_find(sig->hashtable, &m))
            hashtable_add(sig->hashtable, b);
    }
    hashtable_stats_init(sig->hashtable);
    return RS_DONE;
}

void rs_free_sumset(rs_signature_t *psums)
{
    rs_signature_done(psums);
    free(psums);
}

void rs_sumset_dump(rs_signature_t const *sums)
{
    int i;
    rs_block_sig_t *b;
    char strong_hex[RS_MAX_STRONG_SUM_LENGTH * 3];

    rs_log(RS_LOG_INFO | RS_LOG_NONAME,
           "sumset info: magic=%#x, block_len=%d, block_num=%d", sums->magic,
           sums->block_len, sums->count);

    for (i = 0; i < sums->count; i++) {
        b = rs_block_sig_ptr(sums, i);
        rs_hexify(strong_hex, b->strong_sum, sums->strong_sum_len);
        rs_log(RS_LOG_INFO | RS_LOG_NONAME,
               "sum %6d: weak=" FMT_WEAKSUM ", strong=%s", i, b->weak_sum,
               strong_hex);
    }
}
