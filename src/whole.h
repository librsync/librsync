/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2001 by Martin Pool <mbp@sourcefrog.net>
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

/** \file whole.h
 * Whole-file API driver functions. */
#ifndef WHOLE_H
#  define WHOLE_H

#  include <stdio.h>
#  include "librsync.h"

/** Run a job continuously, with input to/from the two specified files.
 *
 * The job should already be set up, and must be freed by the caller after
 * return. If rs_inbuflen or rs_outbuflen are set, they will override the
 * inbuflen and outbuflen arguments.
 *
 * \param job - the job instance to run.
 *
 * \param in_file - input file, or NULL if there is no input.
 *
 * \param out_file - output file, or NULL if there is no output.
 *
 * \param inbuflen - recommended input buffer size to use.
 *
 * \param outbuflen - recommended output buffer size to use.
 *
 * \return RS_DONE if the job completed, or otherwise an error result. */
rs_result rs_whole_run(rs_job_t *job, FILE *in_file, FILE *out_file,
                       int inbuflen, int outbuflen);

#endif                          /* !WHOLE_H */
