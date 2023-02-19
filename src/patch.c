/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
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

                              /*=
                               | This is Tranquility Base.
                               */

/** \file patch.c
 * Apply a delta to an old file to generate a new file. */

#include "config.h"             /* IWYU pragma: keep */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "librsync.h"
#include "job.h"
#include "netint.h"
#include "scoop.h"
#include "command.h"
#include "prototab.h"
#include "trace.h"

static rs_result rs_patch_s_cmdbyte(rs_job_t *);
static rs_result rs_patch_s_params(rs_job_t *);
static rs_result rs_patch_s_run(rs_job_t *);
static rs_result rs_patch_s_literal(rs_job_t *);
static rs_result rs_patch_s_copy(rs_job_t *);
static rs_result rs_patch_s_copying(rs_job_t *);

/** State of trying to read the first byte of a command. Once we've taken that
 * in, we can know how much data to read to get the arguments. */
static rs_result rs_patch_s_cmdbyte(rs_job_t *job)
{
    rs_result result;

    if ((result = rs_suck_byte(job, &job->op)) != RS_DONE)
        return result;
    job->cmd = &rs_prototab[job->op];
    rs_trace("got command %#04x (%s), len_1=%d, len_2=%d", job->op,
             rs_op_kind_name(job->cmd->kind), job->cmd->len_1, job->cmd->len_2);
    if (job->cmd->len_1)
        job->statefn = rs_patch_s_params;
    else {
        job->param1 = job->cmd->immediate;
        job->statefn = rs_patch_s_run;
    }
    return RS_RUNNING;
}

/** Called after reading a command byte to pull in its parameters and then
 * setup to execute the command. */
static rs_result rs_patch_s_params(rs_job_t *job)
{
    rs_result result;
    const size_t len = (size_t)(job->cmd->len_1 + job->cmd->len_2);
    void *p;

    assert(len);
    result = rs_scoop_readahead(job, len, &p);
    if (result != RS_DONE)
        return result;
    /* we now must have LEN bytes buffered */
    result = rs_suck_netint(job, &job->param1, job->cmd->len_1);
    /* shouldn't fail, since we already checked */
    assert(result == RS_DONE);
    if (job->cmd->len_2) {
        result = rs_suck_netint(job, &job->param2, job->cmd->len_2);
        assert(result == RS_DONE);
    }
    job->statefn = rs_patch_s_run;
    return RS_RUNNING;
}

/** Called when we've read in the whole command and we need to execute it. */
static rs_result rs_patch_s_run(rs_job_t *job)
{
    rs_trace("running command %#04x", job->op);
    switch (job->cmd->kind) {
    case RS_KIND_LITERAL:
        job->statefn = rs_patch_s_literal;
        return RS_RUNNING;
    case RS_KIND_END:
        return RS_DONE;
        /* so we exit here; trying to continue causes an error */
    case RS_KIND_COPY:
        job->statefn = rs_patch_s_copy;
        return RS_RUNNING;
    default:
        rs_error("bogus command %#04x", job->op);
        return RS_CORRUPT;
    }
}

/** Called when trying to copy through literal data. */
static rs_result rs_patch_s_literal(rs_job_t *job)
{
    const rs_long_t len = job->param1;
    rs_stats_t *stats = &job->stats;

    rs_trace("LITERAL(length=" FMT_LONG ")", len);
    if (len <= 0 || len > SIZE_MAX) {
        rs_error("invalid length=" FMT_LONG " on LITERAL command", len);
        return RS_CORRUPT;
    }
    stats->lit_cmds++;
    stats->lit_bytes += len;
    stats->lit_cmdbytes += 1 + job->cmd->len_1;
    rs_tube_copy(job, (size_t)len);
    job->statefn = rs_patch_s_cmdbyte;
    return RS_RUNNING;
}

static rs_result rs_patch_s_copy(rs_job_t *job)
{
    const rs_long_t pos = job->param1;
    const rs_long_t len = job->param2;
    rs_stats_t *stats = &job->stats;

    rs_trace("COPY(position=" FMT_LONG ", length=" FMT_LONG ")", pos, len);
    if (len <= 0) {
        rs_error("invalid length=" FMT_LONG " on COPY command", len);
        return RS_CORRUPT;
    }
    if (pos < 0) {
        rs_error("invalid position=" FMT_LONG " on COPY command", pos);
        return RS_CORRUPT;
    }
    stats->copy_cmds++;
    stats->copy_bytes += len;
    stats->copy_cmdbytes += 1 + job->cmd->len_1 + job->cmd->len_2;
    job->basis_pos = pos;
    job->basis_len = len;
    job->statefn = rs_patch_s_copying;
    return RS_RUNNING;
}

/** Called when we're executing a COPY command and waiting for all the data to
 * be retrieved from the callback. */
static rs_result rs_patch_s_copying(rs_job_t *job)
{
    rs_result result;
    rs_buffers_t *buffs = job->stream;
    rs_long_t req = job->basis_len;
    size_t len = buffs->avail_out;
    void *ptr = buffs->next_out;

    /* We are blocked if there is no space left to copy into. */
    if (!len)
        return RS_BLOCKED;
    /* Adjust request to min of amount requested and space available. */
    if ((rs_long_t)len < req)
        req = (rs_long_t)len;
    rs_trace("copy " FMT_LONG " bytes from basis at offset " FMT_LONG "", req,
             job->basis_pos);
    len = (size_t)req;
    result = (job->copy_cb) (job->copy_arg, job->basis_pos, &len, &ptr);
    if (result != RS_DONE) {
        rs_trace("copy callback returned %s", rs_strerror(result));
        return result;
    }
    rs_trace("got " FMT_SIZE " bytes back from basis callback", len);
    /* Actual copied length cannot be greater than requested length. */
    assert(len <= req);
    /* Backwards-compatible defensively handle this for NDEBUG builds. */
    if ((rs_long_t)len > req) {
        rs_warn("copy_cb() returned more than the requested length");
        len = (size_t)req;
    }
    /* copy back to out buffer only if the callback has used its own buffer */
    if (ptr != buffs->next_out)
        memcpy(buffs->next_out, ptr, len);
    /* Update buffs and copy for copied data. */
    buffs->next_out += len;
    buffs->avail_out -= len;
    job->basis_pos += (rs_long_t)len;
    job->basis_len -= (rs_long_t)len;
    if (!job->basis_len) {
        /* Nothing left to copy, we are done! */
        job->statefn = rs_patch_s_cmdbyte;
    }
    return RS_RUNNING;
}

/** Called while we're trying to read the header of the patch. */
static rs_result rs_patch_s_header(rs_job_t *job)
{
    int v;
    rs_result result;

    if ((result = rs_suck_n4(job, &v)) != RS_DONE)
        return result;
    if (v != RS_DELTA_MAGIC) {
        rs_error("got magic number %#x rather than expected value %#x", v,
                 RS_DELTA_MAGIC);
        return RS_BAD_MAGIC;
    } else
        rs_trace("got patch magic %#x", v);
    job->statefn = rs_patch_s_cmdbyte;
    return RS_RUNNING;
}

rs_job_t *rs_patch_begin(rs_copy_cb * copy_cb, void *copy_arg)
{
    rs_job_t *job = rs_job_new("patch", rs_patch_s_header);

    job->copy_cb = copy_cb;
    job->copy_arg = copy_arg;
    rs_mdfour_begin(&job->output_md4);
    return job;
}
