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

/* This provides a compatiblity layer for file operations on different
   platforms. We need to tell IWYU to keep some headers because they are
   required on some platforms but not others. */
#include "config.h"             /* IWYU pragma: keep */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>           /* IWYU pragma: keep */
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>            /* IWYU pragma: keep */
#endif
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>        /* IWYU pragma: keep */
#endif
#ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>         /* IWYU pragma: keep */
#endif
#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>         /* IWYU pragma: keep */
#endif
#ifdef HAVE_IO_H
#  include <io.h>               /* IWYU pragma: keep */
#endif
#include "librsync.h"
#include "trace.h"

/* Use fseeko64, _fseeki64, or fseeko for long files if they exist. */
#if defined(HAVE_FSEEKO64) && (SIZEOF_OFF_T < 8)
#  define fopen(f, m) fopen64((f), (m))
#  define fseek(f, o, w) fseeko64((f), (o), (w))
#elif defined(HAVE__FSEEKI64)
#  define fseek(f, o, w) _fseeki64((f), (o), (w))
#elif defined(HAVE_FSEEKO)
#  define fseek(f, o, w) fseeko((f), (o), (w))
#endif

/* Use fstat64 or _fstati64 for long file fstat if they exist. */
#if defined(HAVE_FSTAT64) && (SIZEOF_OFF_T < 8)
#  define stat stat64
#  define fstat(f,s) fstat64((f), (s))
#elif defined(HAVE__FSTATI64)
#  define stat _stati64
#  define fstat(f,s) _fstati64((f), (s))
#endif

/* Make sure S_ISREG is defined. */
#ifndef S_ISREG
#  define S_ISREG(x) ((x) & _S_IFREG)
#endif

/* Use and prefer _fileno if it exists. */
#ifdef HAVE__FILENO
#  define fileno(f) _fileno((f))
#endif

FILE *rs_file_open(char const *filename, char const *mode, int force)
{
    FILE *f;
    int is_write;

    is_write = mode[0] == 'w';

    if (!filename || !strcmp("-", filename)) {
        if (is_write) {
#if _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
#endif
            return stdout;
        } else {
#if _WIN32
            _setmode(_fileno(stdin), _O_BINARY);
#endif
            return stdin;
        }
    }

    if (!force && is_write) {
        if ((f = fopen(filename, "rb"))) {
            // File exists
            rs_error("File exists \"%s\", aborting!", filename);
            fclose(f);
            exit(RS_IO_ERROR);
        }
    }

    if (!(f = fopen(filename, mode))) {
        rs_error("Error opening \"%s\" for %s: %s", filename,
                 is_write ? "write" : "read", strerror(errno));
        exit(RS_IO_ERROR);
    }

    return f;
}

int rs_file_close(FILE *f)
{
    if ((f == stdin) || (f == stdout))
        return 0;
    return fclose(f);
}

rs_long_t rs_file_size(FILE *f)
{
    struct stat st;
    if ((fstat(fileno(f), &st) == 0) && (S_ISREG(st.st_mode)))
        return st.st_size;
    return -1;
}

rs_result rs_file_copy_cb(void *arg, rs_long_t pos, size_t *len, void **buf)
{
    FILE *f = (FILE *)arg;

    if (fseek(f, pos, SEEK_SET)) {
        rs_error("seek failed: %s", strerror(errno));
        return RS_IO_ERROR;
    }
    *len = fread(*buf, 1, *len, f);
    if (*len) {
        return RS_DONE;
    } else if (ferror(f)) {
        rs_error("read error: %s", strerror(errno));
        return RS_IO_ERROR;
    } else {
        rs_error("unexpected eof on fd%d", fileno(f));
        return RS_INPUT_ENDED;
    }
}
