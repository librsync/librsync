/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2000, 2001 by Martin Pool <mbp@sourcefrog.net>
 * Copyright (C) 2002, 2003 by Donovan Baarda <abo@minkirri.apana.org.au>
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

/** \file mdfour.h
 * MD4 message digest algorithm.
 *
 * \todo Perhaps use the MD4 routine from OpenSSL if it's installed. It's
 * probably not worth the trouble.
 *
 * This was originally written by Andrew Tridgell for use in Samba. It was then
 * modified by;
 *
 * 2002-06-xx: Robert Weber <robert.weber@Colorado.edu> optimisations and fixed
 * >512M support.
 *
 * 2002-06-27: Donovan Baarda <abo@minkirri.apana.org.au> further optimisations
 * and cleanups.
 *
 * 2004-09-09: Simon Law <sfllaw@debian.org> handle little-endian machines that
 * can't do unaligned access (e.g. ia64, pa-risc). */
#ifndef MDFOUR_H
#  define MDFOUR_H

#  include <stdint.h>

/** The rs_mdfour state type. */
struct rs_mdfour {
    unsigned int A, B, C, D;
#  ifdef UINT64_MAX
    uint64_t totalN;
#  else
    uint32_t totalN_hi, totalN_lo;
#  endif
    int tail_len;
    unsigned char tail[64];
};

#endif                          /* !MDFOUR_H */
