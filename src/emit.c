/*=                    -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- dynamic caching and delta update in HTTP
 *
 * Copyright (C) 2000, 2001, 2004 by Martin Pool <mbp@sourcefrog.net>
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

#include "config.h"             /* IWYU pragma: keep */
#include <assert.h>
#include "librsync.h"
#include "emit.h"
#include "job.h"
#include "netint.h"
#include "prototab.h"
#include "trace.h"

void rs_emit_delta_header(rs_job_t *job)
{
    rs_trace("emit DELTA magic");
    rs_squirt_n4(job, RS_DELTA_MAGIC);
}

void rs_emit_literal_cmd(rs_job_t *job, int len)
{
    int cmd;
    int param_len = len <= 64 ? 0 : rs_int_len(len);

    if (param_len == 0) {
        cmd = len;
        rs_trace("emit LITERAL_%d, cmd_byte=%#04x", len, cmd);
    } else if (param_len == 1) {
        cmd = RS_OP_LITERAL_N1;
        rs_trace("emit LITERAL_N1(len=%d), cmd_byte=%#04x", len, cmd);
    } else if (param_len == 2) {
        cmd = RS_OP_LITERAL_N2;
        rs_trace("emit LITERAL_N2(len=%d), cmd_byte=%#04x", len, cmd);
    } else {
        assert(param_len == 4);
        cmd = RS_OP_LITERAL_N4;
        rs_trace("emit LITERAL_N4(len=%d), cmd_byte=%#04x", len, cmd);
    }

    rs_squirt_byte(job, (rs_byte_t)cmd);
    if (param_len)
        rs_squirt_netint(job, len, param_len);

    job->stats.lit_cmds++;
    job->stats.lit_bytes += len;
    job->stats.lit_cmdbytes += 1 + param_len;
}

void rs_emit_copy_cmd(rs_job_t *job, rs_long_t where, rs_long_t len)
{
    int cmd;
    rs_stats_t *stats = &job->stats;
    const int where_bytes = rs_int_len(where);
    const int len_bytes = rs_int_len(len);

    /* Commands ascend (1,1), (1,2), ... (8, 8) */
    if (where_bytes == 8)
        cmd = RS_OP_COPY_N8_N1;
    else if (where_bytes == 4)
        cmd = RS_OP_COPY_N4_N1;
    else if (where_bytes == 2)
        cmd = RS_OP_COPY_N2_N1;
    else {
        assert(where_bytes == 1);
        cmd = RS_OP_COPY_N1_N1;
    }
    if (len_bytes == 1) ;
    else if (len_bytes == 2)
        cmd += 1;
    else if (len_bytes == 4)
        cmd += 2;
    else {
        assert(len_bytes == 8);
        cmd += 3;
    }

    rs_trace("emit COPY_N%d_N%d(where=" FMT_LONG ", len=" FMT_LONG
             "), cmd_byte=%#04x", where_bytes, len_bytes, where, len, cmd);
    rs_squirt_byte(job, (rs_byte_t)cmd);
    rs_squirt_netint(job, where, where_bytes);
    rs_squirt_netint(job, len, len_bytes);

    stats->copy_cmds++;
    stats->copy_bytes += len;
    stats->copy_cmdbytes += 1 + where_bytes + len_bytes;
}

void rs_emit_end_cmd(rs_job_t *job)
{
    int cmd = RS_OP_END;

    rs_trace("emit END, cmd_byte=%#04x", cmd);
    rs_squirt_byte(job, (rs_byte_t)cmd);
}
