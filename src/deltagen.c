/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * deltagen.h -- a basic delta generator.
 *
 * Copyright (C) 2020 by Donovan Baarda <abo@minkirri.apana.org.au>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "deltagen.h"
#include "util.h"
#include "emit.h"
#include "job.h"
#include "stream.h"
#include "command.h"
#include "prototab.h"
#include "trace.h"

rs_deltagen_t *rs_deltagen_new(void *out, rs_send_t *send, rs_stats_t *stats)
{
    rs_deltagen_t *gen = rs_alloc_struct(rs_deltagen_t);

    gen->out = out;
    gen->send = send;
    gen->stats = stats;
    return gen;
}

void rs_deltagen_free(rs_deltagen_t *gen)
{
    free(gen);
}

static inline void rs_deltagen_put_header(rs_deltagen_t *gen)
{
    /* There is nothing queued before init. */
    assert(gen->cmd_len == 0);
    assert(gen->data_len == 0);
    gen->cmd_len = rs_put_delta_header(gen->cmd_buf);
}

static inline void rs_deltagen_put_literal_cmd(rs_deltagen_t *gen, int len)
{
    rs_stats_t *const stats = gen->stats;

    /* There should be nothing queued before emitting a literal cmd. */
    assert(gen->cmd_len == 0);
    assert(gen->data_len == 0);
    gen->cmd_len = rs_put_literal_cmd(len, gen->cmd_buf);
    stats->lit_cmds++;
    stats->lit_bytes += len;
    stats->lit_cmdbytes += gen->cmd_len;
}

static inline void rs_deltagen_put_copy_cmd(rs_deltagen_t *gen, rs_long_t pos,
                                            rs_long_t len)
{
    rs_stats_t *const stats = gen->stats;

    /* There should be nothing queued before emitting a copy cmd. */
    assert(gen->cmd_len == 0);
    assert(gen->data_len == 0);
    gen->cmd_len = rs_put_copy_cmd(pos, len, gen->cmd_buf);
    stats->copy_cmds++;
    stats->copy_bytes += len;
    stats->copy_cmdbytes += gen->cmd_len;
}

static inline void rs_deltagen_put_end_cmd(rs_deltagen_t *gen)
{
    /* This could be appended after another queued cmd. */
    gen->cmd_len += rs_put_end_cmd(gen->cmd_buf + gen->cmd_len);
}

/* Macro for a deltagen to try send and return on errors or blocked. */
#define TRY_DELTAGEN_SEND(len, buf, ret) \
   if ((ret = gen->send(gen->out, len, buf)) <= 0) \
       return ret

/* Macro for a deltagen to try mark and return on errors or blocked. */
#define TRY_DELTAGEN_MARK(mark, ret) \
   if ((ret = rs_deltagen_mark(gen, mark)) <= 0) \
       return ret

/* Handler for sending all data or marks. */
static inline int rs_deltagen_data(rs_deltagen_t *gen, rs_long_t pos, int len,
                                   const void *buf)
{
    int sent_len;

    rs_trace("rs_deltagen_data(pos=" FMT_LONG
             ", len=%d) with cmd_len=%d data_len=%d", pos, len, gen->cmd_len,
             gen->data_len);
    if (!gen->data_len) {
        /* This call is a new call with no incomplete calls pending. */
        gen->data_pos = pos;
        gen->data_len = len;
    } else if (len < 0 && gen->data_len == RS_SEND_SYNC) {
        /* This is a retry of a mark that needs a followup sync. */
        len = RS_SEND_SYNC;
    }
    /* The pos and len arguments must match any pending calls. */
    assert(gen->data_pos == pos);
    assert(gen->data_len == len);
    /* Send init marks before commands. */
    if (len == RS_SEND_INIT) {
        TRY_DELTAGEN_SEND(len, NULL, sent_len);
        /* After init mark sent, change mark to sync to flush the header. */
        len = gen->data_len = RS_SEND_SYNC;
    }
    /* If there's stuff in the cmd buffer, send it. */
    if (gen->cmd_len) {
        TRY_DELTAGEN_SEND(gen->cmd_len, gen->cmd_buf, sent_len);
        gen->cmd_len -= sent_len;
        /* If some is left, shuffle it to the front and return blocked. */
        if (gen->cmd_len) {
            memmove(gen->cmd_buf, gen->cmd_buf + sent_len, gen->cmd_len);
            return 0;
        }
    }
    if (len < 0) {
        /* Send mark. */
        TRY_DELTAGEN_SEND(len, NULL, sent_len);
        assert(gen->data_pos == 0);
        gen->data_len = 0;
    } else if (len > 0) {
        /* Send data. */
        TRY_DELTAGEN_SEND(len, buf, sent_len);
        gen->data_pos += sent_len;
        gen->data_len -= sent_len;
    } else {
        /* There was no mark or data to send. */
        sent_len = 0;
    }
    return sent_len;
}

int rs_deltagen_mark(rs_deltagen_t *gen, int mark)
{
    rs_trace("rs_deltagen_mark(mark=%d) with cmd_len=%d, data_len=%d", mark,
             gen->cmd_len, gen->data_len);
    /* If this is a retry of a pending mark, just finish sending it. */
    if (gen->data_len) {
        assert(gen->data_len == mark || gen->data_len == RS_SEND_SYNC);
        return rs_deltagen_data(gen, 0, mark, NULL);
    }
    /* There should be nothing pending if this is not a retry. */
    assert(gen->cmd_len == 0);
    assert(gen->data_len == 0);
    /* If we have a previous accumulated match, queue and clear it. */
    if (gen->match_len) {
        rs_deltagen_put_copy_cmd(gen, gen->match_pos, gen->match_len);
        gen->match_len = 0;
    }
    /* Handle sending headers/footers. */
    if (mark == RS_SEND_INIT) {
        rs_deltagen_put_header(gen);
    } else if (mark == RS_SEND_DONE) {
        rs_deltagen_put_end_cmd(gen);
    } else {
        assert(mark == RS_SEND_SYNC);
        /* There is no cmd data for a sync. */
    }
    return rs_deltagen_data(gen, 0, mark, NULL);
}

int rs_deltagen_match(rs_deltagen_t *gen, rs_long_t pos, int len,
                      const void *buf)
{
    int sent_len;

    rs_trace("rs_deltagen match(pos= " FMT_LONG " len=%d)", pos, len);
    /* if last was a match that can be extended, extend it */
    if (gen->match_len && (gen->match_pos + gen->match_len) == pos) {
        gen->match_len += (rs_long_t)len;
    } else {
        /* Sync to flush any previously accumulated match. */
        TRY_DELTAGEN_MARK(RS_SEND_SYNC, sent_len);
        /* make this the new match value */
        gen->match_pos = pos;
        gen->match_len = len;
    }
    return len;
}

int rs_deltagen_miss(rs_deltagen_t *gen, rs_long_t pos, int len,
                     const void *buf)
{
    int sent_len;

    rs_trace("rs_deltagen miss(pos=" FMT_LONG " len=%d)", pos, len);
    /* Flush any pending accumulted match and its sync mark first. */
    if (gen->match_len || gen->data_len == RS_SEND_SYNC)
        TRY_DELTAGEN_MARK(RS_SEND_SYNC, sent_len);
    /* If there is no pending miss output, emit the literal cmd. */
    if (!gen->data_len) {
        rs_deltagen_put_literal_cmd(gen, len);
    }
    return rs_deltagen_data(gen, pos, len, buf);
}

int rs_buffers_send(rs_buffers_t *buffers, int len, const void *buf)
{
    rs_trace("rs_buffer_send(len=%d, buffers(avail_out=%d))", len,
             (int)buffers->avail_out);
    /* There is never anything to flush. */
    if (len < 0)
        return 1;
    if ((size_t)len > buffers->avail_out)
        len = (int)buffers->avail_out;
    if (len) {
        memcpy(buffers->next_out, buf, (size_t)len);
        buffers->next_out += len;
        buffers->avail_out -= len;
    }
    return len;
}

int rs_jobstream_send(rs_job_t *job, int len, const void *buf)
{
    return rs_buffers_send(job->stream, len, buf);
}

/* A rs_send_t function that can write to a filo. */
int rs_file_send(FILE *file, int len, const void *buf)
{
    int sent_len;

    /* If len is a flush code, just return success. */
    if (len < 0)
        return 1;
    /* write the data and return an error code if it didn't all get sent. */
    sent_len = (int)fwrite(buf, 1, len, file);
    return (sent_len != len) ? -errno : sent_len;
}

void rs_bufout_init(rs_bufout_t *bufout, void *out, rs_send_t *send)
{
    bufout->out = out;
    bufout->send = send;
    bufout->len = 0;
}

/* Macro for bufout to try send and return on errors or blocked. */
#define TRY_BUFOUT_SEND(len, buf, ret) \
   if ((ret = bufout->send(bufout->out, len, buf)) <= 0) \
       return ret

int rs_bufout_send(rs_bufout_t *bufout, int len, const void *buf)
{
    int sent_len;

    rs_trace("rs_bufout_send(len=%d) with len=%d in buffer", len, bufout->len);
    /* If there is data in the buffer, try to write it first. */
    if (bufout->len) {
        rs_trace("Calling send(len=%d, bufout->buf)", bufout->len);
        TRY_BUFOUT_SEND(bufout->len, bufout->buf, sent_len);
        bufout->len -= sent_len;
        /* If some is left, shuffle it to the front and return blocked. */
        if (bufout->len) {
            memmove(bufout->buf, bufout->buf + sent_len, bufout->len);
            return 0;
        }
    }
    assert(bufout->len == 0);
    /* Try to send the request. */
    rs_trace("Calling send(len=%d, buf)", len);
    TRY_BUFOUT_SEND(len, buf, sent_len);
    /* If any data left would fit in the buffer, append it. */
    if (len > 0 && (len -= sent_len) <= RS_BUFOUT_LEN) {
        memcpy(bufout->buf, (rs_byte_t *)buf + sent_len, len);
        bufout->len = len;
        sent_len += len;
    }
    return sent_len;
}
