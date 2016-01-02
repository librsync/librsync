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


/*
 * TODO: These structures are not terribly useful.  Perhaps we need a
 * splay tree or something that will let us smoothly grow as data is
 * read in.
 */


/**
 * \brief Description of the match described by a signature.
 */
typedef struct rs_target {
    unsigned short  t;
    int             i;
} rs_target_t;

typedef struct rs_block_sig rs_block_sig_t;

/** All tags between l and r inclusive are the same. */
typedef struct rs_tag_table_entry {
    int l; /**< Left bound of the hash tag in sorted array of targets */
    int r; /**< Right bound of the hash tag in sorted array of targets */
} rs_tag_table_entry_t ;

/**
 * This structure describes all the sums generated for an instance of
 * a file.  It incorporates some redundancy to make it easier to
 * search.
 */
struct rs_signature {
    rs_long_t       flength;	/**< Total file length */
    int             count;      /**< How many chunks */
    int             remainder;	/**< flength % block_len */
    int             block_len;
    int             strong_sum_len;
    rs_block_sig_t  *block_sigs; /**< points to info for each chunk */
    rs_tag_table_entry_t	*tag_table;
    rs_target_t     *targets;
    int             magic;
};


/*
 * All blocks are the same length in the current algorithm except for
 * the last block which may be short.
 */
struct rs_block_sig {
    int             i;		    /**< Index of this chunk */
    rs_weak_sum_t   weak_sum;	/**< Simple checksum */
    rs_strong_sum_t strong_sum;	/**< Strong checksum  */
};
