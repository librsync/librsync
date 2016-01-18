/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2016 by Martin Pool
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
 
#include "librsync.h"
#include "search.h"
#include "sumset.h"
#include "trace.h"
#include "util.h"
 
/**
 * Unit tests for \ref search.c
 **/
  

static void test_search_for_block_empty(void) {
    rs_signature_t *sig = rs_alloc_struct(rs_signature_t);
    sig->block_len = 6000;
    rs_stats_t *stats = 0;
    rs_long_t match_where;
    const char *inbuf = "hello";
    
    rs_build_hash_table(sig);
    int result = rs__search_for_block(42, (const rs_byte_t *) inbuf, 6,
        sig, stats, &match_where);
        
    if (result) {
        rs_fatal("unexpectedly found a match at %jd", (intmax_t) match_where);
    }
        
    rs_free_sumset(sig);
}

 
int main(void) {
    test_search_for_block_empty();
    return 0;
}
