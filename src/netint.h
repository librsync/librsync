/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
 *
 * Copyright (C) 1999, 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 1999 by Andrew Tridgell <tridge@samba.org>
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

/** \file netint.h
 * Network-byte-order output to the tube.
 *
 * All the `suck' routines return a result code. The most common values are
 * RS_DONE if they have enough data, or RS_BLOCKED if there is not enough input
 * to proceed.
 *
 * The `squirt` routines also return a result code which in theory could be
 * RS_BLOCKED if there is not enough output space to proceed, but in practice
 * is always RS_DONE. */
#ifndef NETINT_H
#  define NETINT_H

#  include "librsync.h"

/** Write a single byte to a stream output. */
rs_result rs_squirt_byte(rs_job_t *job, rs_byte_t val);

/** Write a variable-length integer to a stream.
 *
 * \param job - Job of data.
 *
 * \param val - Value to write out.
 *
 * \param len - Length of integer, in bytes. */
rs_result rs_squirt_netint(rs_job_t *job, rs_long_t val, int len);

rs_result rs_squirt_n4(rs_job_t *job, int val);

rs_result rs_suck_byte(rs_job_t *job, rs_byte_t *val);

rs_result rs_suck_netint(rs_job_t *job, rs_long_t *val, int len);

rs_result rs_suck_n4(rs_job_t *job, int *val);

int rs_int_len(rs_long_t val);

#endif                          /* !NETINT_H */
