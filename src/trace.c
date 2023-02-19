/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- library for network deltas
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

                                     /*=
                                      | Finality is death.
                                      | Perfection is finality.
                                      | Nothing is perfect.
                                      | There are lumps in it.
                                      */

#include "config.h"             /* IWYU pragma: keep */
#include <stdio.h>
#include <stdarg.h>
#include "librsync.h"
#include "trace.h"
#include "util.h"

rs_trace_fn_t *rs_trace_impl = rs_trace_stderr;

int rs_trace_level = RS_LOG_INFO;

#define MY_NAME "librsync"

static void rs_log_va(int level, char const *fn, char const *fmt, va_list va);

/** Log severity strings, if any. Must match ordering in ::rs_loglevel. */
static const char *rs_severities[] = {
    "EMERGENCY! ", "ALERT! ", "CRITICAL! ", "ERROR: ", "Warning: ",
    "", "", ""
};

/** Set the destination of trace information.
 *
 * The callback scheme allows for use within applications that may have their
 * own particular ways of reporting errors: log files for a web server,
 * perhaps, and an error dialog for a browser.
 *
 * \todo Do we really need such fine-grained control, or just yes/no tracing? */
void rs_trace_to(rs_trace_fn_t *new_impl)
{
    rs_trace_impl = new_impl;
}

void rs_trace_set_level(rs_loglevel level)
{
    rs_trace_level = level;
}

static void rs_log_va(int flags, char const *fn, char const *fmt, va_list va)
{
    int level = flags & RS_LOG_PRIMASK;

    if (rs_trace_impl && level <= rs_trace_level) {
        char buf[1000];
        char full_buf[1040];

        vsnprintf(buf, sizeof(buf), fmt, va);
        if (flags & RS_LOG_NONAME || !(*fn)) {
            snprintf(full_buf, sizeof(full_buf), "%s: %s%s\n", MY_NAME,
                     rs_severities[level], buf);
        } else {
            snprintf(full_buf, sizeof(full_buf), "%s: %s(%s) %s\n", MY_NAME,
                     rs_severities[level], fn, buf);
        }
        rs_trace_impl(level, full_buf);
    }
}

/* Called by a macro that prepends the calling function name, etc. */
void rs_log0(int level, char const *fn, char const *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    rs_log_va(level, fn, fmt, va);
    va_end(va);
}

void rs_trace_stderr(rs_loglevel UNUSED(level), char const *msg)
{
    fputs(msg, stderr);
}

int rs_supports_trace(void)
{
#ifdef DO_RS_TRACE
    return 1;
#else
    return 0;
#endif                          /* !DO_RS_TRACE */
}
