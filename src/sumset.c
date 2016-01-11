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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "librsync.h"
#include "sumset.h"
#include "util.h"
#include "trace.h"


rs_result rs__sumset_append(
    rs_signature_t *sig,
    rs_weak_sum_t weak_sum,
    const rs_strong_sum_t *strong_sum)
{
    size_t              new_size;
    rs_block_sig_t      *asignature;

    sig->count++;
    new_size = sig->count * sizeof(rs_block_sig_t);

    sig->block_sigs = realloc(sig->block_sigs, new_size);

    if (sig->block_sigs == NULL) {
        return RS_MEM_ERROR;
    }
    asignature = &(sig->block_sigs[sig->count - 1]);

    asignature->weak_sum = weak_sum;
    asignature->i = sig->count;

    memcpy(asignature->strong_sum, strong_sum, sig->strong_sum_len);

    if (rs_trace_enabled()) {
        char                hexbuf[RS_MAX_STRONG_SUM_LENGTH * 2 + 2];
        rs_hexify(hexbuf, strong_sum, sig->strong_sum_len);

        rs_trace("read in checksum: weak=%#x, strong=%s", asignature->weak_sum,
                 hexbuf);
    }
    return RS_DONE;
}


void
rs_free_sumset(rs_signature_t * psums)
{
        if (psums->block_sigs)
                free(psums->block_sigs);

        if (psums->tag_table)
		free(psums->tag_table);

        if (psums->targets)
                free(psums->targets);

        rs_bzero(psums, sizeof *psums);
        free(psums);
}


void
rs_sumset_dump(rs_signature_t const *sums)
{
        int i;
        char        strong_hex[RS_MAX_STRONG_SUM_LENGTH * 3];
    
        rs_log(RS_LOG_INFO,
                "sumset info: block_len=%d, file length=%lu, "
                "number of chunks=%d, remainder=%d",
                sums->block_len,
                (unsigned long) sums->flength, sums->count,
                sums->remainder);

        for (i = 0; i < sums->count; i++) {
                rs_hexify(strong_hex, sums->block_sigs[i].strong_sum,
                          sums->strong_sum_len);
                rs_log(RS_LOG_INFO,
                        "sum %6d: weak=%08x, strong=%s",
                        i, sums->block_sigs[i].weak_sum, strong_hex);
        }
}
