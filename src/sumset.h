/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 * 
 * Copyright (C) 2016 by Martin Nowak <code@dawg.eu>
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

/**
 * \brief Hash bucket
 */
typedef struct rs_hash_bucket {
    rs_weak_sum_t weak_sum;
    unsigned int strong_idx;
} rs_hash_bucket_t;

/*
 * All blocks are the same length in the current algorithm except for
 * the last block which may be short.
 */
typedef struct rs_block_sig {
    rs_weak_sum_t   weak_sum;	/* simple checksum */
    rs_strong_sum_t strong_sum;	/* checksum  */
} rs_block_sig_t;

/*
 * This structure describes all the sums generated for an instance of
 * a file.  It incorporates some redundancy to make it easier to
 * search.
 */
struct rs_signature {
    rs_hash_bucket_t *buckets; /* hash table to look up weak sums */
    rs_strong_sum_t *strong_sums; /* strong hash sums */
    unsigned int bucket_len;
    int strong_sum_len;
    rs_block_sig_t *block_sigs; /* as read from on-disk format */
    rs_long_t       flength;	/* total file length */
    int             magic;
    int             count;      /* how many chunks */
    int             remainder;	/* flength % block_length */
    int             block_len;	/* block_length */
};
