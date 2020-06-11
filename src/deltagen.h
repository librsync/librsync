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
#ifndef _DELTAGEN_H_
#  define _DELTAGEN_H_

#  include "librsync.h"

/** Special constants for rs_send_t and rs_genmark_t arguments for flushing. */
#  define RS_SEND_INIT -1
#  define RS_SEND_SYNC -2
#  define RS_SEND_DONE -3

/** Function type for writing data.
 *
 * This can be used for writing data to an output. They should return the
 * amount of data actually transferred, which must be less than or equal to the
 * length requested. A zero return value indicates output is blocked. A
 * negative return value indicates an error (usually negative errno.h values).
 *
 * When sending data, len can be a special negative RS_SEND_* value to flush
 * accumulated data to initialize, synchronise, or finalize the output. When
 * len is negative the return value will be zero if the flush didn't finish,
 * positive if flushing completed, or negative for errors. The flush should be
 * called repeatedly until it returns non-zero to ensure it completes. Flush
 * operations should idempotent once they complete (additional calls do nothing
 * extra).
 *
 * /param obj - the reader/writer/getter/putter object to use.
 *
 * /param len - the amount of data to transfer or negative flush code.
 *
 * /param *buf - the data buffer to transfer to/from, ignored for flushes. */
typedef int rs_send_t(void *obj, int len, const void *buf);

/** Callback type used for generating a "mark" output for a delta.
 *
 * These are used for emitting "events" into the delta output like init, sync,
 * and done markers. The `mark` argument should be set to a RS_SEND_* value.
 *
 * \param gen - the delta generator object.
 *
 * \param mark - the event marker type.
 *
 * \returns - positive for success, zero for blocked (try again), or negative
 * for errors. */
typedef int rs_genmark_t(void *gen, int mark);

/** Callback types used for creating or applying hit/miss data.
 *
 * These are used both to "write" hit/miss data into a delta output, and to
 * "read" copy data when doing a patch.
 *
 * \param gen - the delta generator object.
 *
 * \param pos - Position where the match/miss/get/put is.
 *
 * \param len - Length of match/miss/get/put data.
 *
 * \param buf - Data for the match/miss/get/put.
 *
 * \returns - Count of data actually processed, or negative values for errors. */
typedef int rs_gendata_t(void *gen, rs_long_t pos, int len, const void *buf);

/** A delta generator that generates a standard librsync delta. */
typedef struct rs_deltagen_s {
    void *out;                  /**< The writer object to use. */
    rs_send_t *send;            /**< The send function to use. */
    rs_long_t match_pos;        /**< The last accumulated match position. */
    rs_long_t match_len;        /**< The last accumulated match length. */
    rs_long_t data_pos;         /**< The data position left to output. */
    int data_len;               /**< The data length left to output. */
    int cmd_len;                /**< The cmd length to send. */
    rs_byte_t cmd_buf[64];      /**< The cmd buffer to send. */
} rs_deltagen_t;
rs_deltagen_t *rs_deltagen_new(void *out, rs_send_t *send);
void rs_deltagen_free(rs_deltagen_t *gen);
int rs_deltagen_mark(rs_deltagen_t *gen, int mark);
int rs_deltagen_match(rs_deltagen_t *gen, rs_long_t pos, int len,
                      const void *buf);
int rs_deltagen_miss(rs_deltagen_t *gen, rs_long_t pos, int len,
                     const void *buf);

/** An rs_send_t function for writing data to a rs_buffers_t. */
int rs_buffers_send(rs_buffers_t *buffers, int len, const void *buf);

/** An rs_send_t function for writing data to a job's stream. */
int rs_jobstream_send(rs_job_t *job, int len, const void *buf);

/* A rs_send_t function that can write to a filo. */
int rs_file_send(FILE *out, int len, const void *buf);

#  define RS_BUFOUT_LEN 64
/** An output writer that wraps another writer and adds a buffer.
 *
 * This writer adds a small buffer to guarantee that writes smaller than 64
 * bytes are atomic and will write all or nothing. This means small cmd writes
 * that block can be just re-tried without needing to handle partial writes. */
typedef struct rs_bufout {
    void *out;
    rs_send_t *send;
    int len;
    rs_byte_t buf[RS_BUFOUT_LEN];
} rs_bufout_t;
void rs_bufout_init(rs_bufout_t *bufout, void *out, rs_send_t *send);
int rs_bufout_send(rs_bufout_t *bufout, int len, const void *buf);

/* Create a mksig job using generator for creating the delta output. */
rs_job_t *rs_job_mksig(void *gen, rs_genmark_t *mark_cb,
                       rs_gendata_t *block_cb);

/* Create a delta job using generator for creating the delta output. */
rs_job_t *rs_job_delta(rs_signature_t *sig, void *gen, rs_genmark_t *mark_cb,
                       rs_gendata_t *match_cb, rs_gendata_t *miss_cb);

/* Create a delta job using generator for creating the delta output. */
rs_job_t *rs_job_patch(void *gen, rs_genmark_t *mark_cb,
                       rs_gendata_t *match_cb, rs_gendata_t *miss_cb);

/* A rs_send_t function that can drive a job by just sending data to it. */
int rs_job_send(rs_job_t *job, int len, const void *buf);

#endif                          /* _DELTAGEN_H_ */
