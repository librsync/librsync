/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
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
                               | Pick a window, Jimmy, you're leaving.
                               */

#include "config.h"             /* IWYU pragma: keep */
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "librsync.h"
#include "buf.h"
#include "job.h"
#include "trace.h"
#include "util.h"

struct rs_filebuf {
    FILE *f;
    char *buf;
    size_t buf_len;
};

rs_filebuf_t *rs_filebuf_new(FILE *f, size_t buf_len)
{
    rs_filebuf_t *pf = rs_alloc_struct(rs_filebuf_t);

    pf->buf = rs_alloc(buf_len, "file buffer");
    pf->buf_len = buf_len;
    pf->f = f;
    return pf;
}

void rs_filebuf_free(rs_filebuf_t *fb)
{
    free(fb->buf);
    rs_bzero(fb, sizeof *fb);
    free(fb);
}

/* If the stream has no more data available, read some from F into BUF, and let
   the stream use that. On return, SEEN_EOF is true if the end of file has
   passed into the stream. */
rs_result rs_infilebuf_fill(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
    size_t len;
    rs_filebuf_t *fb = (rs_filebuf_t *)opaque;
    FILE *f = fb->f;

    /* If buf has data, it must be in the buffer. */
    if (buf->avail_in) {
        assert(buf->next_in >= fb->buf);
        assert(buf->next_in + buf->avail_in <= fb->buf + fb->buf_len);
    }
    if (buf->eof_in) {
        return RS_DONE;
    } else if (buf->avail_in > fb->buf_len / 2) {
        /* Buf is already full enough, do nothing. */
        return RS_DONE;
    } else if (buf->avail_in) {
        /* Some leftover tail data, move it to the front of the buffer. */
        rs_trace("moving buffer " FMT_SIZE " bytes to reuse " FMT_SIZE " bytes",
                 buf->avail_in, (size_t)(buf->next_in - fb->buf));
        memmove(fb->buf, buf->next_in, buf->avail_in);
    }
    buf->next_in = fb->buf;
    len = fread(fb->buf + buf->avail_in, 1, fb->buf_len - buf->avail_in, f);
    if (len == 0) {
        if ((buf->eof_in = feof(f))) {
            rs_trace("seen end of file on input");
            return RS_DONE;
        } else {
            rs_error("error filling buf from file: %s", strerror(errno));
            return RS_IO_ERROR;
        }
    }
    buf->avail_in += len;
    job->stats.in_bytes += len;
    return RS_DONE;
}

/* The buf is already using BUF for an output buffer, and probably contains
   some buffered output now. Write this out to F, and reset the buffer cursor. */
rs_result rs_outfilebuf_drain(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
    rs_filebuf_t *fb = (rs_filebuf_t *)opaque;
    FILE *f = fb->f;

    /* If next_out is NULL, we haven't pointed it at fb->buf yet. */
    if (!buf->next_out) {
        assert(buf->avail_out == 0);
        buf->next_out = fb->buf;
        buf->avail_out = fb->buf_len;
    }
    /* The buf output buffer must be at the end of fb->buf. */
    assert(buf->next_out >= fb->buf);
    assert(buf->next_out + buf->avail_out == fb->buf + fb->buf_len);

    size_t present = buf->next_out - fb->buf;
    if (present > 0) {
        size_t result = fwrite(fb->buf, 1, present, f);
        if (present != result) {
            rs_error("error draining buf to file: %s", strerror(errno));
            return RS_IO_ERROR;
        }
        buf->next_out = fb->buf;
        buf->avail_out = fb->buf_len;
        job->stats.out_bytes += result;
    }
    return RS_DONE;
}
