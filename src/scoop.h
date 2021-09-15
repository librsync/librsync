/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
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
                 | Two wars in a lifetime bear hard on the little places.
                 | In winter when storms come rushing out of the dark,
                 | And the bay boils like a cauldron of sharks,
                 | The old remember the trenches at Paschendale
                 | And sons who died on the Burma Railway.
                 */

/** \file scoop.h
 * Manage librsync streams of IO.
 *
 * See \sa scoop.c and \sa tube.c for related code for input and output
 * respectively.
 *
 * OK, so I'll admit IO here is a little complex. The most important player
 * here is the stream, which is an object for managing filter operations. It
 * has both input and output sides, both of which is just a (pointer,len) pair
 * into a buffer provided by the client. The code controlling the stream
 * handles however much data it wants, and the client provides or accepts
 * however much is convenient.
 *
 * At the same time as being friendly to the client, we also try to be very
 * friendly to the internal code. It wants to be able to ask for arbitrary
 * amounts of input or output and get it without having to keep track of
 * partial completion. So there are functions which either complete, or queue
 * whatever was not sent and return RS_BLOCKED.
 *
 * The output buffer is a little more clever than simply a data buffer. Instead
 * it knows that we can send either literal data, or data copied through from
 * the input of the stream.
 *
 * In buf.c you will find functions that then map buffers onto stdio files.
 *
 * On return from an encoding function, some input will have been consumed
 * and/or some output produced, but either the input or the output or possibly
 * both could have some remaining bytes available.
 *
 * librsync never does IO or memory allocation for stream input, but relies on
 * the caller. This is very nice for integration, but means that we have to be
 * fairly flexible as to when we can `read' or `write' stuff internally.
 *
 * librsync basically does two types of IO. It reads network integers of
 * various lengths which encode command and control information such as
 * versions and signatures. It also does bulk data transfer.
 *
 * IO of network integers is internally buffered, because higher levels of the
 * code need to see them transmitted atomically: it's no good to read half of a
 * uint32. So there is a small and fixed length internal buffer which
 * accumulates these. Unlike previous versions of the library, we don't require
 * that the caller hold the start until the whole thing has arrived, which
 * guarantees that we can always make progress.
 *
 * On each call into a stream iterator, it should begin by trying to flush
 * output. This may well use up all the remaining stream space, in which case
 * nothing else can be done. */
#ifndef SCOOP_H
#  define SCOOP_H
#  include <stdbool.h>
#  include <stddef.h>
#  include "job.h"
#  include "librsync.h"

rs_result rs_tube_catchup(rs_job_t *job);
int rs_tube_is_idle(rs_job_t const *job);
void rs_tube_write(rs_job_t *job, void const *buf, size_t len);
void rs_tube_copy(rs_job_t *job, size_t len);

void rs_scoop_advance(rs_job_t *job, size_t len);
rs_result rs_scoop_readahead(rs_job_t *job, size_t len, void **ptr);
rs_result rs_scoop_read(rs_job_t *job, size_t len, void **ptr);
rs_result rs_scoop_read_rest(rs_job_t *job, size_t *len, void **ptr);

static inline size_t rs_scoop_avail(rs_job_t *job)
{
    return job->scoop_avail + job->stream->avail_in;
}

/** Test if the scoop has reached eof. */
static inline bool rs_scoop_eof(rs_job_t *job)
{
    return !rs_scoop_avail(job) && job->stream->eof_in;
}

/** Get a pointer to the next input in the scoop. */
static inline void *rs_scoop_buf(rs_job_t *job)
{
    return job->scoop_avail ? (void *)job->scoop_next : (void *)job->
        stream->next_in;
}

/** Get the contiguous length of the next input in the scoop. */
static inline size_t rs_scoop_len(rs_job_t *job)
{
    return job->scoop_avail ? job->scoop_avail : job->stream->avail_in;
}

/** Get the next contiguous buffer of data available in the scoop.
 *
 * This will return a pointer to the data and reduce len to the amount of
 * contiguous data available at that position.
 *
 * \param *job - the job instance to use.
 *
 * \param *len - the amount of data desired, updated to the amount available.
 *
 * \return A pointer to the data. */
static inline void *rs_scoop_getbuf(rs_job_t *job, size_t *len)
{
    size_t max_len = rs_scoop_len(job);
    if (*len > max_len)
        *len = max_len;
    return rs_scoop_buf(job);
}

/** Iterate through and consume contiguous data buffers in the scoop.
 *
 * Example: \code
 *   size_t len=rs_scoop_avail(job);
 *   size_t ilen;
 *
 *   for (buf = rs_scoop_iterbuf(job, &len, &ilen); ilen > 0;
 *        buf = rs_scoop_nextbuf(job, &len, &ilen))
 *     ilen = fwrite(buf, ilen, 1, f);
 * \endcode
 *
 * At each iteration buf and ilen are the data and its length for the current
 * iteration, and len is the remaining data to iterate through including the
 * current iteration. During an iteration you can change ilen to indicate only
 * part of the buffer was processed and the next iteration will take this into
 * account. Setting ilen = 0 to indicate blocking or errors will terminate
 * iteration.
 *
 * At the end of iteration buf will point at the next location in the scoop
 * after the iterated data, len and ilen will be zero, or the remaining data
 * and last ilen if iteration was terminated by setting ilen = 0.
 *
 * \param *job - the job instance to use.
 *
 * \param *len - the size_t amount of data to iterate over.
 *
 * \param *ilen - the size_t amount of data in the current iteration.
 *
 * \return A pointer to data in the current iteration. */
static inline void *rs_scoop_iterbuf(rs_job_t *job, size_t *len, size_t *ilen)
{
    *ilen = *len;
    return rs_scoop_getbuf(job, ilen);
}

/** Get the next iteration of contiguous data buffers from the scoop.
 *
 * This advances the scoop for the previous iteration, and gets the next
 * iteration. \sa rs_scoop_iterbuf */
static inline void *rs_scoop_nextbuf(rs_job_t *job, size_t *len, size_t *ilen)
{
    if (*ilen == 0)
        return rs_scoop_buf(job);
    rs_scoop_advance(job, *ilen);
    *len -= *ilen;
    return rs_scoop_iterbuf(job, len, ilen);
}

#endif                          /* !SCOOP_H */
