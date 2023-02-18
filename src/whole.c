/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright 2000, 2001, 2014, 2015 by Martin Pool <mbp@sourcefrog.net>
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
                               | Is it possible that software is not
                               | like anything else, that it is meant
                               | to be discarded: that the whole point
                               | is to always see it as a soap bubble?
                               |        -- Alan Perlis
                               */

#include "config.h"             /* IWYU pragma: keep */
#include <stdio.h>
#include <string.h>
#include "librsync.h"
#include "whole.h"
#include "sumset.h"
#include "job.h"
#include "buf.h"
#include "librsync_export.h"

/** Whole file IO buffer sizes. */
LIBRSYNC_EXPORT int rs_inbuflen = 0, rs_outbuflen = 0;

rs_result rs_whole_run(rs_job_t *job, FILE *in_file, FILE *out_file,
                       int inbuflen, int outbuflen)
{
    rs_buffers_t buf;
    rs_result result;
    rs_filebuf_t *in_fb = NULL, *out_fb = NULL;

    /* Override buffer sizes if rs_inbuflen or rs_outbuflen are set. */
    inbuflen = rs_inbuflen ? rs_inbuflen : inbuflen;
    outbuflen = rs_outbuflen ? rs_outbuflen : outbuflen;
    if (in_file)
        in_fb = rs_filebuf_new(in_file, inbuflen);
    if (out_file)
        out_fb = rs_filebuf_new(out_file, outbuflen);
    result =
        rs_job_drive(job, &buf, in_fb ? rs_infilebuf_fill : NULL, in_fb,
                     out_fb ? rs_outfilebuf_drain : NULL, out_fb);
    if (in_fb)
        rs_filebuf_free(in_fb);
    if (out_fb)
        rs_filebuf_free(out_fb);
    return result;
}

rs_result rs_sig_file(FILE *old_file, FILE *sig_file, size_t block_len,
                      size_t strong_len, rs_magic_number sig_magic,
                      rs_stats_t *stats)
{
    rs_job_t *job;
    rs_result r;
    rs_long_t old_fsize = rs_file_size(old_file);

    if ((r =
         rs_sig_args(old_fsize, &sig_magic, &block_len,
                     &strong_len)) != RS_DONE)
        return r;
    job = rs_sig_begin(block_len, strong_len, sig_magic);
    /* Size inbuf for 4 blocks, outbuf for header + 4 blocksums. */
    r = rs_whole_run(job, old_file, sig_file, 4 * (int)block_len,
                     12 + 4 * (4 + (int)strong_len));
    if (stats)
        memcpy(stats, &job->stats, sizeof *stats);
    rs_job_free(job);

    return r;
}

rs_result rs_loadsig_file(FILE *sig_file, rs_signature_t **sumset,
                          rs_stats_t *stats)
{
    rs_job_t *job;
    rs_result r;

    job = rs_loadsig_begin(sumset);
    /* Set filesize used to estimate signature size. */
    job->sig_fsize = rs_file_size(sig_file);
    /* Size inbuf for 1024x 16 byte blocksums. */
    r = rs_whole_run(job, sig_file, NULL, 1024 * 16, 0);
    if (stats)
        memcpy(stats, &job->stats, sizeof *stats);
    rs_job_free(job);

    return r;
}

rs_result rs_delta_file(rs_signature_t *sig, FILE *new_file, FILE *delta_file,
                        rs_stats_t *stats)
{
    rs_job_t *job;
    rs_result r;

    job = rs_delta_begin(sig);
    /* Size inbuf for 4*(CMD + 1 block), outbuf for 4*CMD. */
    r = rs_whole_run(job, new_file, delta_file,
                     4 * (MAX_DELTA_CMD + sig->block_len), 4 * MAX_DELTA_CMD);
    if (stats)
        memcpy(stats, &job->stats, sizeof *stats);
    rs_job_free(job);
    return r;
}

rs_result rs_patch_file(FILE *basis_file, FILE *delta_file, FILE *new_file,
                        rs_stats_t *stats)
{
    rs_job_t *job;
    rs_result r;

    job = rs_patch_begin(rs_file_copy_cb, basis_file);
    /* Default size inbuf 1*CMD and outbuf 4*CMD. */
    r = rs_whole_run(job, delta_file, new_file, MAX_DELTA_CMD,
                     4 * MAX_DELTA_CMD);
    if (stats)
        memcpy(stats, &job->stats, sizeof *stats);
    rs_job_free(job);
    return r;
}
