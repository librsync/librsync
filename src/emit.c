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

/** \file emit.c
 * encoding output routines.
 *
 * \todo Pluggable encoding formats: gdiff-style, rsync 24, ed (text), Delta
 * HTTP. */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include "librsync.h"
#include "emit.h"
#include "netint.h"
#include "command.h"
#include "prototab.h"
#include "trace.h"

/** Write the magic for the start of a delta. */
int rs_put_delta_header(rs_byte_t *buf)
{
    rs_trace("emit DELTA magic");
    return rs_put_netint(RS_DELTA_MAGIC, 4, buf);
}

/** Write a LITERAL command. */
int rs_put_literal_cmd(int len, rs_byte_t *buf)
{
    int cmd;
    int param_len = len <= 64 ? 0 : rs_int_len(len);

    if (param_len == 0) {
        cmd = (int)len;
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
    *buf++ = (rs_byte_t)cmd;
    if (param_len)
        rs_put_netint(len, param_len, buf);
    return 1 + param_len;
}

/** Write a COPY command for given offset and length.
 *
 * There is a choice of variable-length encodings, depending on the size of
 * representation for the parameters. */
int rs_put_copy_cmd(rs_long_t pos, rs_long_t len, rs_byte_t *buf)
{
    int cmd;
    const int pos_bytes = rs_int_len(pos);
    const int len_bytes = rs_int_len(len);

    /* Commands ascend (1,1), (1,2), ... (8, 8) */
    if (pos_bytes == 8)
        cmd = RS_OP_COPY_N8_N1;
    else if (pos_bytes == 4)
        cmd = RS_OP_COPY_N4_N1;
    else if (pos_bytes == 2)
        cmd = RS_OP_COPY_N2_N1;
    else {
        assert(pos_bytes == 1);
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
    rs_trace("emit COPY_N%d_N%d(pos=" FMT_LONG ", len=" FMT_LONG
             "), cmd_byte=%#04x", pos_bytes, len_bytes, pos, len, cmd);
    *buf++ = (rs_byte_t)cmd;
    buf += rs_put_netint(pos, pos_bytes, buf);
    buf += rs_put_netint(len, len_bytes, buf);
    return 1 + pos_bytes + len_bytes;
}

/** Write an END command. */
int rs_put_end_cmd(rs_byte_t *buf)
{
    int cmd = RS_OP_END;

    rs_trace("emit END, cmd_byte=%#04x", cmd);
    *buf = (rs_byte_t)cmd;
    return 1;
}
