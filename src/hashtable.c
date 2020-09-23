/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * hashtable.c -- a generic hashtable implementation.
 *
 * Copyright (C) 2016 by Donovan Baarda <abo@minkirri.apana.org.au>
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
#include <assert.h>
#include <stdlib.h>
#include "hashtable.h"

/* Open addressing works best if it can take advantage of memory caches using
   locality for probes of adjacent buckets on collisions. So we pack the keys
   tightly together in their own key table and avoid referencing the element
   table and elements as much as possible. Key value zero is reserved as a
   marker for an empty bucket to avoid checking for NULL in the element table.
   If we do get a hash value of zero, we -1 to wrap it around to 0xffff. */

/* Use max 0.7 load factor to avoid bad open addressing performance. */
#define HASHTABLE_LOADFACTOR_NUM 7
#define HASHTABLE_LOADFACTOR_DEN 10

hashtable_t *_hashtable_new(int size)
{
    hashtable_t *t;
    unsigned size2, bits2;

    /* Adjust requested size to account for max load factor. */
    size = 1 + size * HASHTABLE_LOADFACTOR_DEN / HASHTABLE_LOADFACTOR_NUM;
    /* Use next power of 2 larger than the requested size and get mask bits. */
    for (size2 = 2, bits2 = 1; (int)size2 < size; size2 <<= 1, bits2++) ;
    if (!(t = calloc(1, sizeof(hashtable_t)+ size2 * sizeof(unsigned))))
        return NULL;
    if (!(t->etable = calloc(size2, sizeof(void *)))) {
        _hashtable_free(t);
        return NULL;
    }
    t->size = (int)size2;
    t->count = 0;
    t->tmask = size2 - 1;
#ifndef HASHTABLE_NBLOOM
    if (!(t->kbloom = calloc((size2 + 7) / 8, sizeof(unsigned char)))) {
        _hashtable_free(t);
        return NULL;
    }
    t->bshift = (unsigned)sizeof(unsigned) * 8 - bits2;
    assert(t->tmask == (unsigned)-1 >> t->bshift);
#endif
#ifndef HASHTABLE_NSTATS
    t->find_count = t->match_count = t->hashcmp_count = t->entrycmp_count = 0;
#endif
    return t;
}

void _hashtable_free(hashtable_t *t)
{
    if (t) {
        free(t->etable);
#ifndef HASHTABLE_NBLOOM
        free(t->kbloom);
#endif
        free(t);
    }
}
