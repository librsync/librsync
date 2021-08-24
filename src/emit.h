/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- dynamic caching and delta update in HTTP
 *
 * Copyright (C) 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
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

/** \file emit.h
 * encoding output routines.
 *
 * \todo Pluggable encoding formats: gdiff-style, rsync 24, ed (text), Delta
 * HTTP. */
#ifndef EMIT_H
#  define EMIT_H

#  include "librsync.h"

/** Write the magic for the start of a delta. */
void rs_emit_delta_header(rs_job_t *);

/** Write a LITERAL command. */
void rs_emit_literal_cmd(rs_job_t *, int len);

/** Write a COPY command for given offset and length.
 *
 * There is a choice of variable-length encodings, depending on the size of
 * representation for the parameters. */
void rs_emit_copy_cmd(rs_job_t *job, rs_long_t where, rs_long_t len);

/** Write an END command. */
void rs_emit_end_cmd(rs_job_t *);

#endif                          /* !EMIT_H */
