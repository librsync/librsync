/*=                                     -*- c-file-style: "bsd" -*-
 *
 * libhsync -- library for network deltas
 * $Id$
 * 
 * Copyright (C) 2000 by Martin Pool <mbp@samba.org>
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


/*
 * Test driver for libhsync stream functions.
 */

#include "config.h"

#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>

#include "includes.h"
#include "private.h"
#include "util.h"
#include "stream.h"
#include "file.h"
#include "nozzle.h"
#include "streamcpy.h"


static void do_copy(FILE *in_file, FILE *out_file, int inbuflen, int outbuflen)
{
    hs_nozzle_t *in_iobuf, *out_iobuf;
    hs_stream_t stream;

    hs_stream_init(&stream);

    in_iobuf = _hs_nozzle_new(in_file, &stream, inbuflen, "r");
    out_iobuf = _hs_nozzle_new(out_file, &stream, outbuflen, "w");

    _hs_stream_copy_file(&stream, in_iobuf, out_iobuf);

    _hs_nozzle_delete(in_iobuf);
    _hs_nozzle_delete(out_iobuf);
}



int main(int argc, char **argv)
{
    int inbuflen, outbuflen;
    
    if (argc != 3) {
        fprintf(stderr, "usage: dstream INBUF OUTBUF\n");
        return 1;
    }

    inbuflen = atoi(argv[1]);
    outbuflen = atoi(argv[2]);
    
    do_copy(stdin, stdout, inbuflen, outbuflen);
    
    return 0;
}