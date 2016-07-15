/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2016 by Martin Nowak <code@dawg.eu>
 * Copyright (C) 1999, 2000, 2001, 2014 by Martin Pool <mbp@sourcefrog.net>
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

/*
 * This file contains code for searching the sumset for matching
 * values.
 */

/*
 * TODO: The common case is that the next block in both streams
 * match. Can we make that a bit faster at all?  We'd need to perhaps
 * add a link forward between blocks in the sum_struct corresponding
 * to the order they're found in the input; then before doing a search
 * we can just check that pointer.
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

// maximum load factor of hash table
#define LOAD_FACTOR_NUM 7
#define LOAD_FACTOR_DEN 10

static unsigned int next_pow_2(unsigned int v)
{
    if (v < 2) return v;
    return (1U << (32 - __builtin_clz(v - 1)));
}

static bool insert_entry(rs_signature_t *sigs, const rs_block_sig_t *block_sig, unsigned int strong_idx)
{
    const unsigned int mask = sigs->bucket_len - 1;
    size_t i, step;
    for (i = block_sig->weak_sum & mask, step = 0; ; i = (i + ++step) & mask)
    {
        if (sigs->buckets[i].strong_idx == 0) // empty bucket
        {
            sigs->buckets[i].weak_sum = block_sig->weak_sum;
            sigs->buckets[i].strong_idx = strong_idx + 1;
            memcpy(sigs->strong_sums[strong_idx], block_sig->strong_sum, sigs->strong_sum_len);
            return true;
        }
        else if (sigs->buckets[i].weak_sum == block_sig->weak_sum &&
                 memcmp(block_sig->strong_sum, sigs->strong_sums[sigs->buckets[i].strong_idx - 1], sigs->strong_sum_len) == 0)
        {
            return false;
        }

    }
}

static unsigned int lookup_entry(const rs_signature_t *sigs, rs_weak_sum_t weak_sum, const rs_byte_t *inbuf, size_t block_len)
{
    rs_strong_sum_t strong_sum;
    bool has_strong_sum = false;

    const unsigned int mask = sigs->bucket_len - 1;
    size_t i, step;
    for (i = weak_sum & mask, step = 0; ; i = (i + ++step) & mask)
    {
        if (sigs->buckets[i].strong_idx == 0) // empty bucket
            return -1;
        if (sigs->buckets[i].weak_sum != weak_sum)
            continue;
        if (!has_strong_sum)
        {
            if (sigs->magic == RS_BLAKE2_SIG_MAGIC)
                rs_calc_blake2_sum(inbuf, block_len, &strong_sum);
            else if (sigs->magic == RS_MD4_SIG_MAGIC)
                rs_calc_md4_sum(inbuf, block_len, &strong_sum);
            else
            {
                /* Bad input data is checked in rs_delta_begin, so this
                 * should never be reached. */
                rs_fatal("Unknown signature algorithm %#x", sigs->magic);
                return -1;
            }
            has_strong_sum = true;
        }

        if (memcmp(strong_sum, sigs->strong_sums[sigs->buckets[i].strong_idx - 1], sigs->strong_sum_len) == 0)
            return sigs->buckets[i].strong_idx - 1;
    }
}

rs_result rs_build_hash_table(rs_signature_t *sigs)
{
    sigs->bucket_len = sigs->count ? next_pow_2((sigs->count * LOAD_FACTOR_DEN + LOAD_FACTOR_NUM - 1) / LOAD_FACTOR_NUM) : 1;
    sigs->buckets = calloc(sigs->bucket_len, sizeof(sigs->buckets[0]));
    if (!sigs->buckets)
        return RS_MEM_ERROR;

    sigs->strong_sums = calloc(sigs->count, sizeof(sigs->strong_sums[0]));
    if (!sigs->strong_sums)
    {
        free(sigs->buckets);
        sigs->buckets = NULL;
        return RS_MEM_ERROR;
    }

    size_t strong_idx = 0;
    size_t i;
    for (i = 0; i < sigs->count; ++i)
    {
        if (insert_entry(sigs, &sigs->block_sigs[i], strong_idx)) // sort out duplicates
            ++strong_idx;
    }

    // no longer needed
    free(sigs->block_sigs);
    sigs->block_sigs = NULL;

    rs_trace("rs_build_hash_table done");
    return RS_DONE;
}


/*
 * See if there is a match for the specified block INBUF..BLOCK_LEN in
 * the checksum set, using precalculated WEAK_SUM.
 *
 * If we don't find a match on the weak checksum, then we just give
 * up.  If we do find a weak match, then we proceed to calculate the
 * strong checksum for the current block, and see if it will match
 * anything.
 */
bool rs_search_for_block(rs_weak_sum_t weak_sum,
                    const rs_byte_t *inbuf,
                    size_t block_len,
                    rs_signature_t const *sigs, rs_stats_t *stats,
                    rs_long_t * match_where)
{
    /* Caller must have called rs_build_hash_table() by now */
    if (!sigs->buckets)
        rs_fatal("Must have called rs_build_hash_table() by now");

    const unsigned int strong_idx = lookup_entry(sigs, weak_sum, inbuf, block_len);
    if (strong_idx == -1)
        return false;

    *match_where = (rs_long_t) strong_idx * sigs->block_len;
    return true;
}
