/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2000, 2001, 2014 by Martin Pool <mbp@sourcefrog.net>
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

/** \file job.h
 * Generic state-machine interface.
 *
 * The point of this is that we need to be able to suspend and resume
 * processing at any point at which the buffers may block.
 *
 * \sa \ref api_streaming \sa rs_job_iter() \sa ::rs_job */
#ifndef JOB_H
#  define JOB_H

#  include <assert.h>
#  include <stddef.h>
#  include "mdfour.h"
#  include "checksum.h"
#  include "librsync.h"

/** Magic job tag number for checking jobs have been initialized. */
#  define RS_JOB_TAG 20010225

/** Max length of a singled delta command is including command bytes.
 *
 * This is used to constrain and set the internal buffer sizes. */
#  define MAX_DELTA_CMD (1<<16)

/** The contents of this structure are private. */
struct rs_job {
    int dogtag;

    /** Human-readable job operation name. */
    const char *job_name;

    rs_buffers_t *stream;

    /** Callback for each processing step. */
    rs_result (*statefn)(rs_job_t *);

    /** Final result of processing job. Used by rs_job_s_failed(). */
    rs_result final_result;

    /* Arguments for initializing the signature used by mksum.c and readsums.c.
     */
    int sig_magic;
    int sig_block_len;
    int sig_strong_len;

    /** The size of the signature file if available. Used by loadsums.c when
     * initializing the signature to preallocate memory. */
    rs_long_t sig_fsize;

    /** Pointer to the signature that's being used by the operation. */
    rs_signature_t *signature;

    /** Flag indicating signature should be destroyed with the job. */
    int job_owns_sig;

    /** Command byte currently being processed, if any. */
    unsigned char op;

    /** The weak signature digest used by readsums.c */
    rs_weak_sum_t weak_sig;

    /** The rollsum weak signature accumulator used by delta.c */
    weaksum_t weak_sum;

    /** Lengths of expected parameters. */
    rs_long_t param1, param2;

    struct rs_prototab_ent const *cmd;
    rs_mdfour_t output_md4;

    /** Encoding statistics. */
    rs_stats_t stats;

    /** Buffer of data in the scoop. Allocation is scoop_buf[0..scoop_alloc],
     * and scoop_next[0..scoop_avail] contains data yet to be processed. */
    rs_byte_t *scoop_buf;       /**< The buffer allocation pointer. */
    rs_byte_t *scoop_next;      /**< The next data pointer. */
    size_t scoop_alloc;         /**< The buffer allocation size. */
    size_t scoop_avail;         /**< The amount of data available. */

    /** The delta scan buffer, where scan_buf[scan_pos..scan_len] is the data
     * yet to be scanned. */
    rs_byte_t *scan_buf;        /**< The delta scan buffer pointer. */
    size_t scan_len;            /**< The delta scan buffer length. */
    size_t scan_pos;            /**< The delta scan position. */

    /** If USED is >0, then buf contains that much write data to be sent out. */
    rs_byte_t write_buf[36];
    size_t write_len;

    /** If \p copy_len is >0, then that much data should be copied through
     * from the input. */
    size_t copy_len;

    /** Copy from the basis position. */
    rs_long_t basis_pos, basis_len;

    /** Callback used to copy data from the basis into the output. */
    rs_copy_cb *copy_cb;
    void *copy_arg;
};

rs_job_t *rs_job_new(const char *, rs_result (*statefn)(rs_job_t *));

/** Assert that a job is valid.
 *
 * We don't use a static inline function here so that assert failure output
 * points at where rs_job_check() was called from. */
#  define rs_job_check(job) do {\
    assert(job->dogtag == RS_JOB_TAG);\
} while (0)

#endif                          /* !JOB_H */
