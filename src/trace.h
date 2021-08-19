/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- generate and apply network deltas
 *
 * Copyright (C) 2000, 2001, 2004 by Martin Pool <mbp@sourcefrog.net>
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

/** \file trace.h
 * logging functions.
 *
 * trace may be turned off.
 *
 * error is always on, but you can return and continue in some way.
 *
 * fatal terminates the whole process.
 *
 * \todo A function like perror that includes strerror output. Apache does this
 * by adding flags as well as the severity level which say whether such
 * information should be included. */
#ifndef TRACE_H
#  define TRACE_H

#  include <inttypes.h>
#  include "config.h"

/* Printf format patters for standard librsync types. */
#  define FMT_LONG "%"PRIdMAX
#  define FMT_WEAKSUM "%08"PRIx32
/* Old MSVC compilers don't support "%zu" and have "%Iu" instead. */
#  ifdef HAVE_PRINTF_Z
#    define FMT_SIZE "%zu"
#  else
#    define FMT_SIZE "%Iu"
#  endif

/* Some old compilers don't support __func_ and have __FUNCTION__ instead. */
#  ifndef HAVE___FUNC__
#    ifdef HAVE___FUNCTION__
#      define __func__ __FUNCTION__
#    else
#      define __func__ ""
#    endif
#  endif

/* Non-GNUC compatible compilers don't support __attribute__(). */
#  ifndef __GNUC__
#    define __attribute__(x)
#  endif

void rs_log0(int level, char const *fn, char const *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/** \def rs_trace_enabled()
 * Call this before putting too much effort into generating trace messages. */
#  ifdef DO_RS_TRACE
#    define rs_trace_enabled() ((rs_trace_level & RS_LOG_PRIMASK) >= RS_LOG_DEBUG)
#    define rs_trace(...) rs_log0(RS_LOG_DEBUG, __func__, __VA_ARGS__)
#  else
#    define rs_trace_enabled() 0
#    define rs_trace(...)
#  endif                        /* !DO_RS_TRACE */

#  define rs_log(l, ...) rs_log0((l), __func__, __VA_ARGS__)
#  define rs_warn(...) rs_log0(RS_LOG_WARNING, __func__, __VA_ARGS__)
#  define rs_error(...) rs_log0(RS_LOG_ERR,  __func__, __VA_ARGS__)
#  define rs_fatal(...) do { \
    rs_log0(RS_LOG_CRIT, __func__, __VA_ARGS__); \
    abort(); \
} while (0)

enum {
    RS_LOG_PRIMASK = 7,         /**< Mask to extract priority part. \internal */
    RS_LOG_NONAME = 8           /**< \b Don't show function name in message. */
};

extern int rs_trace_level;

#endif                          /* !TRACE_H */
