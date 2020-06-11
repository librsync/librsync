/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
 *
 * Copyright (C) 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 2003 by Donovan Baarda <abo@minkirri.apana.org.au>
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

                                /*=
                                 | Let's climb to the TOP of that
                                 | MOUNTAIN and think about STRIP
                                 | MINING!!
                                 */

/** \file delta.c
 * Generate in streaming mode an rsync delta given a set of signatures, and a
 * new file.
 *
 * The size of blocks for signature generation is determined by the block size
 * in the incoming signature.
 *
 * To calculate a signature, we need to be able to see at least one block of
 * the new file at a time. Once we have that, we calculate its weak signature,
 * and see if there is any block in the signature hash table that has the same
 * weak sum. If there is one, then we also compute the strong sum of the new
 * block, and cross check that. If they're the same, then we can assume we have
 * a match.
 *
 * The final block of the file has to be handled a little differently, because
 * it may be a short match. Short blocks in the signature don't include their
 * length -- we just allow for the final short block of the file to match any
 * block in the signature, and if they have the same checksum we assume they
 * must have the same length. Therefore, when we emit a COPY command, we have
 * to send it with a length that is the same as the block matched, and not the
 * block length from the signature.
 *
 * Profiling results as of v1.26, 2001-03-18:
 *
 * If everything matches, then we spend almost all our time in rs_mdfour64 and
 * rs_weak_sum, which is unavoidable and therefore a good profile.
 *
 * If nothing matches, it is not so good.
 *
 * 2002-06-26: Donovan Baarda
 *
 * The following is based entirely on pysync. It is much cleaner than the
 * previous incarnation of this code. It is slightly complicated because in
 * this case the output can block, so the main delta loop needs to stop when
 * this happens.
 *
 * In pysync a 'last' attribute is used to hold the last miss or match for
 * extending if possible. In this code, basis_len and scoop_pos are used
 * instead of 'last'. When basis_len > 0, last is a match. When basis_len = 0
 * and scoop_pos is > 0, last is a miss. When both are 0, last is None (ie,
 * nothing).
 *
 * Pysync is also slightly different in that a 'flush' method is available to
 * force output of accumulated data. This 'flush' is use to finalise delta
 * calculation. In librsync input is terminated with an eof flag on the input
 * stream. I have structured this code similar to pysync with a seperate flush
 * function that is used when eof is reached. This allows for a flush style API
 * if one is ever needed. Note that flush in pysync can be used for more than
 * just terminating delta calculation, so a flush based API can in some ways be
 * more flexible...
 *
 * The input data is first scanned, then processed. Scanning identifies input
 * data as misses or matches, and emits the instruction stream. Processing the
 * data consumes it off the input scoop and outputs the processed miss data
 * into the tube.
 *
 * The scoop contains all data yet to be processed. The scoop_pos is an index
 * into the scoop that indicates the point scanned to. As data is scanned,
 * scoop_pos is incremented. As data is processed, it is removed from the scoop
 * and scoop_pos adjusted. Everything gets complicated because the tube can
 * block. When the tube is blocked, no data can be processed. */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include "librsync.h"
#include "job.h"
#include "deltagen.h"
#include "sumset.h"
#include "checksum.h"
#include "stream.h"
#include "emit.h"
#include "trace.h"

static rs_result rs_delta_s_scan(rs_job_t *job);
static rs_result rs_delta_s_flush(rs_job_t *job);
static rs_result rs_delta_s_end(rs_job_t *job);
static inline void rs_getinput(rs_job_t *job);
static inline rs_result rs_putoutput(rs_job_t *job);
static inline int rs_findmatch(rs_job_t *job, rs_long_t *match_pos,
                               size_t *match_len);
static inline rs_result rs_appendmatch(rs_job_t *job, rs_long_t match_pos,
                                       size_t match_len);
static inline rs_result rs_appendmiss(rs_job_t *job, size_t miss_len);
static inline rs_result rs_appendflush(rs_job_t *job);
static inline rs_result rs_processmatch(rs_job_t *job);
static inline rs_result rs_processmiss(rs_job_t *job);

/* Max miss/match length for 0.01% 3 literal command bytes overhead. */
#define MAX_DATA_LEN 32768

/** Get a block of data if possible, and see if it matches.
 *
 * On each call, we try to process all of the input data available on the scoop
 * and input buffer. */
static rs_result rs_delta_s_scan(rs_job_t *job)
{
    const size_t block_len = job->signature->block_len;
    rs_long_t match_pos;
    size_t match_len;
    rs_result result;

    rs_job_check(job);
    /* Read the input into the scoop. */
    rs_getinput(job);
    /* Send any pending flushed output. */
    result = rs_putoutput(job);
    /* while output is not blocked and there is a block of data */
    while ((result == RS_DONE)
           && ((job->scoop_pos + block_len) < job->scoop_avail)) {
        /* check if this block matches */
        if (rs_findmatch(job, &match_pos, &match_len)) {
            /* append the match and reset the weak_sum */
            result = rs_appendmatch(job, match_pos, match_len);
            weaksum_reset(&job->weak_sum);
        } else {
            /* rotate the weak_sum and append the miss byte */
            weaksum_rotate(&job->weak_sum, job->scoop_next[job->scoop_pos],
                           job->scoop_next[job->scoop_pos + block_len]);
            result = rs_appendmiss(job, 1);
        }
    }
    /* if we completed OK */
    if (result == RS_DONE) {
        /* if we reached eof, we can flush the last fragment */
        if (job->stream->eof_in) {
            job->statefn = rs_delta_s_flush;
            return RS_RUNNING;
        } else {
            /* we are blocked waiting for more data */
            return RS_BLOCKED;
        }
    }
    return result;
}

static rs_result rs_delta_s_flush(rs_job_t *job)
{
    rs_long_t match_pos;
    size_t match_len;
    rs_result result;

    rs_job_check(job);
    /* Read the input into the scoop. */
    rs_getinput(job);
    /* Send any pending flushed output. */
    result = rs_putoutput(job);
    /* while output is not blocked and there is any remaining data */
    while ((result == RS_DONE) && (job->scoop_pos < job->scoop_avail)) {
        /* check if this block matches */
        if (rs_findmatch(job, &match_pos, &match_len)) {
            /* append the match and reset the weak_sum */
            result = rs_appendmatch(job, match_pos, match_len);
            weaksum_reset(&job->weak_sum);
        } else {
            /* rollout from weak_sum and append the miss byte */
            weaksum_rollout(&job->weak_sum, job->scoop_next[job->scoop_pos]);
            rs_trace("block reduced to " FMT_SIZE "",
                     weaksum_count(&job->weak_sum));
            result = rs_appendmiss(job, 1);
        }
    }
    /* if we are not blocked, flush and set end statefn. */
    if (result == RS_DONE) {
        result = rs_appendflush(job);
        if (result == RS_DONE) {
            job->statefn = rs_delta_s_end;
            return RS_RUNNING;
        }
    }
    return result;
}

static rs_result rs_delta_s_end(rs_job_t *job)
{
    int res;
    rs_trace("Calling mark_cb(RS_SEND_DONE)");
    if ((res = job->mark_cb(job->gen, RS_SEND_DONE)) <= 0)
        return res ? RS_IO_ERROR : RS_BLOCKED;
    return RS_DONE;
}

static inline void rs_getinput(rs_job_t *job)
{
    size_t len;

    len = rs_scoop_total_avail(job);
    if (job->scoop_avail < len) {
        rs_scoop_input(job, len);
    }
}

static inline rs_result rs_putoutput(rs_job_t *job)
{
    /* Output any pending match or miss data. */
    if (job->match_len) {
        rs_trace("sending %d match data", job->match_len);
        assert(!job->miss_len);
        return rs_processmatch(job);
    } else if (job->miss_len) {
        rs_trace("sending %d miss data", job->miss_len);
        assert(!job->match_len);
        return rs_processmiss(job);
    }
    /* otherwise, nothing to flush so we are done */
    return RS_DONE;
}

/** find a match at scoop_pos, returning the match_pos and match_len.
 *
 * Note that this will calculate weak_sum if required. It will also determine
 * the match_len.
 *
 * This routine could be modified to do xdelta style matches that would extend
 * matches past block boundaries by matching backwards and forwards beyond the
 * block boundaries. Extending backwards would require decrementing scoop_pos
 * as appropriate. */
static inline int rs_findmatch(rs_job_t *job, rs_long_t *match_pos,
                               size_t *match_len)
{
    const size_t block_len = job->signature->block_len;

    /* calculate the weak_sum if we don't have one */
    if (weaksum_count(&job->weak_sum) == 0) {
        /* set match_len to min(block_len, scan_avail) */
        *match_len = job->scoop_avail - job->scoop_pos;
        if (*match_len > block_len) {
            *match_len = block_len;
        }
        /* Update the weak_sum */
        weaksum_update(&job->weak_sum, job->scoop_next + job->scoop_pos,
                       *match_len);
        rs_trace("calculate weak sum from scratch length " FMT_SIZE "",
                 weaksum_count(&job->weak_sum));
    } else {
        /* set the match_len to the weak_sum count */
        *match_len = weaksum_count(&job->weak_sum);
    }
    *match_pos =
        rs_signature_find_match(job->signature, weaksum_digest(&job->weak_sum),
                                job->scoop_next + job->scoop_pos, *match_len);
    return *match_pos != -1;
}

/** Append a match at match_pos of length match_len to the delta, extending a
 * previous match if possible, or flushing any previous miss/match. */
static inline rs_result rs_appendmatch(rs_job_t *job, rs_long_t match_pos,
                                       size_t match_len)
{
    rs_result result = RS_DONE;

    /* if last was a match that can be extended, extend it */
    if (job->basis_len && (job->basis_pos + job->basis_len) == match_pos
        && job->basis_len < MAX_DATA_LEN) {
        job->basis_len += match_len;
    } else {
        /* else appendflush the last value */
        result = rs_appendflush(job);
        /* make this the new match value */
        job->basis_pos = match_pos;
        job->basis_len = (int)match_len;
    }
    /* increment scoop_pos to point at next unscanned data */
    job->scoop_pos += match_len;
    return result;
}

/** Append a miss of length miss_len to the delta, extending a previous miss
 * if possible, or flushing any previous match.
 *
 * This also breaks misses up into 32KB segments to avoid accumulating too much
 * in memory. */
static inline rs_result rs_appendmiss(rs_job_t *job, size_t miss_len)
{
    rs_result result = RS_DONE;

    /* If last was a match, or MAX_DATA_LEN misses, appendflush it. */
    if (job->basis_len || (job->scoop_pos >= MAX_DATA_LEN)) {
        result = rs_appendflush(job);
    }
    /* increment scoop_pos */
    job->scoop_pos += miss_len;
    return result;
}

/** Flush any accumulating hit or miss, appending it to the delta. */
static inline rs_result rs_appendflush(rs_job_t *job)
{
    /* Set flush match or miss pos/len to the last match or miss */
    if (job->basis_len) {
        rs_trace("found match " FMT_SIZE " bytes at " FMT_LONG " from " FMT_LONG
                 "!", job->scoop_pos, job->input_pos, job->basis_pos);
        job->match_pos = job->basis_pos;
        job->match_len = job->basis_len;
        job->basis_len = 0;
    } else if (job->scoop_pos) {
        rs_trace("found miss " FMT_SIZE " bytes at " FMT_LONG "!",
                 job->scoop_pos, job->input_pos);
        job->miss_len = (int)job->scoop_pos;
    }
    /* Reset the scoop_pos for after the data is flushed. */
    job->scoop_pos = 0;
    return rs_putoutput(job);
}

/** Process matching data in the scoop.
 *
 * The scoop contains match data at scoop_next of length scoop_pos. This
 * function processes that match data, returning RS_DONE if it completes, or
 * RS_BLOCKED if it gets blocked. After it completes scoop_pos is reset to
 * still point at the next unscanned data.
 *
 * This function currently just removes data from the scoop and adjusts
 * scoop_pos appropriately. In the future this could be used for something like
 * context compressing of miss data. Note that it also calls rs_tube_catchup to
 * output any pending output. */
static inline rs_result rs_processmatch(rs_job_t *job)
{
    rs_long_t pos = job->match_pos;
    int len = job->match_len;
    void *buf = job->scoop_next;
    int sent_len;

    rs_trace("Calling match_cb(gen, " FMT_LONG ", %d, buf)", pos, len);
    if ((sent_len = job->match_cb(job->gen, pos, len, buf)) <= 0)
        return sent_len ? RS_IO_ERROR : RS_BLOCKED;
    job->scoop_avail -= sent_len;
    job->scoop_next += sent_len;
    job->input_pos += sent_len;
    job->match_pos += sent_len;
    job->match_len -= sent_len;
    return job->match_len ? RS_BLOCKED : RS_DONE;
}

/** Process miss data in the scoop.
 *
 * The scoop contains miss data at scoop_next of length scoop_pos. This
 * function processes that miss data, returning RS_DONE if it completes, or
 * RS_BLOCKED if it gets blocked. After it completes scoop_pos is reset to
 * still point at the next unscanned data.
 *
 * This function uses rs_tube_copy to queue copying from the scoop into output.
 * and uses rs_tube_catchup to do the copying. This automaticly removes data
 * from the scoop, but this can block. While rs_tube_catchup is blocked,
 * scoop_pos does not point at legit data, so scanning can also not proceed.
 *
 * In the future this could do compression of miss data before outputing it. */
static inline rs_result rs_processmiss(rs_job_t *job)
{
    rs_long_t pos = job->input_pos;
    int len = job->miss_len;
    void *buf = job->scoop_next;
    int sent_len;

    rs_trace("Calling miss_cb(gen, " FMT_LONG ", %d, buf)", pos, len);
    if ((sent_len = job->miss_cb(job->gen, pos, len, buf)) <= 0)
        return sent_len ? RS_IO_ERROR : RS_BLOCKED;
    job->scoop_avail -= sent_len;
    job->scoop_next += sent_len;
    job->input_pos += sent_len;
    job->miss_len -= sent_len;
    return job->miss_len ? RS_BLOCKED : RS_DONE;
}

/** State function that does a slack delta containing only literal data to
 * recreate the input. */
static rs_result rs_delta_s_slack(rs_job_t *job)
{
    rs_buffers_t *const stream = job->stream;
    rs_long_t pos = job->input_pos;
    int len = (int)stream->avail_in;
    void *buf = stream->next_in;
    int sent_len;

    if (len) {
        rs_trace("Calling miss_cb(gen, " FMT_LONG ", %d, buf)", pos, len);
        if ((sent_len = job->miss_cb(job->gen, pos, len, buf)) <= 0)
            return sent_len ? RS_IO_ERROR : RS_BLOCKED;
        job->input_pos += sent_len;
        stream->avail_in -= sent_len;
        stream->next_in += sent_len;
    } else if (rs_job_input_is_ending(job)) {
        job->statefn = rs_delta_s_end;
        return RS_RUNNING;
    }
    return RS_BLOCKED;
}

/** State function for writing out the header of the encoding job. */
static rs_result rs_delta_s_header(rs_job_t *job)
{
    int res;

    rs_trace("Calling mark_cb(RS_SEND_INIT)");
    if ((res = job->mark_cb(job->gen, RS_SEND_INIT)) <= 0)
        return res ? RS_IO_ERROR : RS_BLOCKED;
    job->input_pos = 0;
    if (job->signature) {
        job->statefn = rs_delta_s_scan;
    } else {
        rs_trace("no signature provided for delta, using slack deltas");
        job->statefn = rs_delta_s_slack;
    }
    return RS_RUNNING;
}

rs_job_t *rs_job_delta(rs_signature_t *sig, void *gen, rs_genmark_t *mark_cb,
                       rs_gendata_t *match_cb, rs_gendata_t *miss_cb)
{
    rs_job_t *job;

    job = rs_job_new("delta", rs_delta_s_header);
    /* Caller can pass NULL sig or empty sig for "slack deltas". */
    if (sig && sig->count > 0) {
        rs_signature_check(sig);
        /* Caller must have called rs_build_hash_table() by now. */
        assert(sig->hashtable);
        job->signature = sig;
        weaksum_init(&job->weak_sum, rs_signature_weaksum_kind(sig));
    }
    /* Setup delta output generator callbacks. */
    job->gen = gen;
    job->mark_cb = mark_cb;
    job->match_cb = match_cb;
    job->miss_cb = miss_cb;
    return job;
}

rs_job_t *rs_delta_begin(rs_signature_t *sig)
{
    /* We initialize the job with gen=NULL and set it after, because we need
       the job to create the deltagen, since it needs the job to send its
       output. */
    rs_job_t *job = rs_job_delta(sig, NULL, (rs_genmark_t *)rs_deltagen_mark,
                                 (rs_gendata_t *)rs_deltagen_match,
                                 (rs_gendata_t *)rs_deltagen_miss);
    rs_deltagen_t *gen = rs_deltagen_new(job, (rs_send_t *)rs_jobstream_send);
    job->gen = gen;
    return job;
}
