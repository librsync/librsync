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

/** \file stream.h
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
 * So on return from an encoding function, either the input or the output or
 * possibly both will have no more bytes available.
 *
 * librsync never does IO or memory allocation, but relies on the caller. This
 * is very nice for integration, but means that we have to be fairly flexible
 * as to when we can `read' or `write' stuff internally.
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
#ifndef STREAM_H
#  define STREAM_H

size_t rs_buffers_copy(rs_buffers_t *stream, size_t len);

rs_result rs_tube_catchup(rs_job_t *job);
int rs_tube_is_idle(rs_job_t const *job);
void rs_tube_write(rs_job_t *job, void const *buf, size_t len);
void rs_tube_copy(rs_job_t *job, size_t len);

void rs_scoop_input(rs_job_t *job, size_t len);
void rs_scoop_advance(rs_job_t *job, size_t len);
rs_result rs_scoop_readahead(rs_job_t *job, size_t len, void **ptr);
rs_result rs_scoop_read(rs_job_t *job, size_t len, void **ptr);
rs_result rs_scoop_read_rest(rs_job_t *job, size_t *len, void **ptr);
size_t rs_scoop_total_avail(rs_job_t *job);

endif                           /* !STREAM_H */
