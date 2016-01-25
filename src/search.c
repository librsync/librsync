/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 1999, 2000, 2001, 2014, 2015, 2016 by Martin Pool
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

/**
 * \file
 *
 * Searching an \ref ::rs_signature for matching values.
 */


#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "librsync.h"
#include "trace.h"
#include "util.h"
#include "sumset.h"
#include "search.h"
#include "checksum.h"

#define TABLE_SIZE (1<<16)
#define NULL_TAG (-1)

#define gettag2(s1,s2) (((s1) + (s2)) & 0xFFFF)
#define gettag(sum) gettag2((sum)&0xFFFF,(sum)>>16)

static void swap(rs_target_t *t1, rs_target_t *t2) {
    unsigned short ts = t1->t;
    t1->t = t2->t;
    t2->t = ts;

    int ti = t1->i;
    t1->i = t2->i;
    t2->i = ti;
}

static int rs_compare_targets(rs_target_t const *t1, rs_target_t const *t2, rs_signature_t * sums) {
    int v = (int) t1->t - (int) t2->t;
    if (v != 0)
        return v;

    rs_weak_sum_t w1 = sums->block_sigs[t1->i].weak_sum;
    rs_weak_sum_t w2 = sums->block_sigs[t2->i].weak_sum;

    v = (w1 > w2) - (w1 < w2);
    if (v != 0)
        return v;

    return memcmp(sums->block_sigs[t1->i].strong_sum,
            sums->block_sigs[t2->i].strong_sum,
            sums->strong_sum_len);
}

static void heap_sort(rs_signature_t * sums) {
    unsigned int i, j, n, k, p;
    for (i = 1; i < sums->count; ++i) {
        for (j = i; j > 0;) {
            p = (j - 1) >> 1;
            if (rs_compare_targets(&sums->targets[j], &sums->targets[p], sums) > 0)
                swap(&sums->targets[j], &sums->targets[p]);
            else
                break;
            j = p;
        }
    }

    for (n = sums->count - 1; n > 0;) {
        swap(&sums->targets[0], &sums->targets[n]);
        --n;
        for (i = 0; ((i << 1) + 1) <= n;) {
            k = (i << 1) + 1;
            if ((k + 1 <= n) && (rs_compare_targets(&sums->targets[k], &sums->targets[k + 1], sums) < 0))
                k = k + 1;
            if (rs_compare_targets(&sums->targets[k], &sums->targets[i], sums) > 0)
                swap(&sums->targets[k], &sums->targets[i]);
            else
                break;
            i = k;
        }
    }
}

rs_result
rs_build_hash_table(rs_signature_t * sums)
{
    int i;

    sums->tag_table = calloc(TABLE_SIZE, sizeof(sums->tag_table[0]));
    if (!sums->tag_table)
        return RS_MEM_ERROR;

    if (sums->count > 0) {
        sums->targets = calloc(sums->count, sizeof(rs_target_t));
        if (!sums->targets) {
            free(sums->tag_table);
            sums->tag_table = NULL;
            return RS_MEM_ERROR;
        }

        for (i = 0; i < sums->count; i++) {
            sums->targets[i].i = i;
            sums->targets[i].t = gettag(sums->block_sigs[i].weak_sum);
        }

        heap_sort(sums);
    }

    for (i = 0; i < TABLE_SIZE; i++) {
        sums->tag_table[i].l = NULL_TAG;
        sums->tag_table[i].r = NULL_TAG;
    }

    for (i = sums->count - 1; i >= 0; i--) {
        sums->tag_table[sums->targets[i].t].l = i;
    }

    for (i = 0; i < sums->count; i++) {
        sums->tag_table[sums->targets[i].t].r = i;
    }

    rs_trace("rs_build_hash_table done");
    return RS_DONE;
}


/**
 * \private
 *
 * See if there is a match for the specified block \p inbuf in
 * the checksum set, using precalculated \p weak_sum.
 *
 * If we don't find a match on the weak checksum, then we just give
 * up.  If we do find a weak match, then we proceed to calculate the
 * strong checksum for the current block, and see if it will match
 * anything.
 *
 * This does a binary search on a two-part key of the weak sum and then the
 * strong sum.
 *
 * \param sig In-memory signature to search.
 *
 * \param inbuf_len Number of bytes to match from \p inbuf: normally the
 * signature block len, unless this is a short final block.
 *
 * \returns True if an exact match was found.
 */
int
rs__search_for_block(
    rs_weak_sum_t weak_sum,
    const rs_byte_t *inbuf,
    size_t inbuf_len,
    rs_signature_t const *sig,
    rs_stats_t * stats,
    rs_long_t * match_where)
{
    /* Caller must have called rs_build_hash_table() by now */
    if (!sig->tag_table) {
        rs_fatal("Must have called rs_build_hash_table() by now");
        return 0;
    }

    rs_strong_sum_t strong_sum;
    int got_strong = 0; /* strong sum of inbuf has been calculated. */
    int hash_tag = gettag(weak_sum);
    rs_tag_table_entry_t *bucket = &(sig->tag_table[hash_tag]);
    int l = bucket->l; /* Left inclusive bound of search region */
    int r = bucket->r; /* Right inclusive bound of search region */
    int v;  /* direction of next move: -ve left, +ve right */

    if (l == NULL_TAG)
        return 0;

    while (1) {
        if (r < l) {
            rs_fatal("bisection range inverted [%d, %d]", l, r);
            return 0;
        }
        int m = (l + r) >> 1; /* midpoint of search region */
        if (m < 0 || m >= sig->count) {
            rs_fatal("bisection m=%d out of range [0,%d]", m, sig->count);
            return 0;
        }
        int i = sig->targets[m].i;
        const rs_block_sig_t *b = &(sig->block_sigs[i]);
        v = (weak_sum > b->weak_sum) - (weak_sum < b->weak_sum);
        // v < 0  - weak_sum <  b->weak_sum
        // v == 0 - weak_sum == b->weak_sum
        // v > 0  - weak_sum >  b->weak_sum
        
        if (l == r && v != 0) {
            /* Weak sum doesn't match and there's no other options to search. */
            return 0;
        }

        if (v == 0) {
            if (!got_strong) {
                /* Lazy calculate strong sum after finding weak match. */
                if(sig->magic == RS_BLAKE2_SIG_MAGIC) {
                    rs_calc_blake2_sum(inbuf, inbuf_len, &strong_sum);
                } else if (sig->magic == RS_MD4_SIG_MAGIC) {
                    rs_calc_md4_sum(inbuf, inbuf_len, &strong_sum);
                } else {
                    rs_fatal("Unknown signature algorithm %#x", sig->magic);
                    return 0;
                }
                got_strong = 1;
            }
            v = memcmp(strong_sum, b->strong_sum, sig->strong_sum_len);
            if (v == 0) {
                int token = b->i;
                *match_where = (rs_long_t)(token - 1) * sig->block_len;
                return 1;
            }
        }

        /* Mismatched on either the weak or strong sum: continue to bisect into
         * the range, if any remains. */
        if (l == r)
            return 0;
        else if (v > 0) {
            l = m + 1;
        } else {
            r = m - 1;
        }
    }
    return 0;
}
