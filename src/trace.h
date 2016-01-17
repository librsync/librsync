/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- generate and apply network deltas
 *
 * Copyright (C) 2000, 2001, 2004, 2016 by Martin Pool
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


/*
 * TODO: A function like perror that includes strerror output.  Apache
 * does this by adding flags as well as the severity level which say
 * whether such information should be included.
 */


/*
 * trace may be turned off.
 *
 * error is always on, but you can return and continue in some way
 *
 * fatal terminates the whole process
 */



/* There is no portable way in C99 to printf 64-bit types.  Many
 * platforms do have a format which will do it, but it's not
 * standardized.  Therefore these macros.
 *
 * Not all platforms using gnu C necessarily have a corresponding
 * printf, but it's probably a good starting point.  Most unix systems
 * seem to use %ll.
 */
#if SIZEOF_LONG == 8
#  define PRINTF_CAST_U64(x) ((unsigned long) (x))
#  define PRINTF_FORMAT_U64 "%lu"
#elif defined(__GNUC__) || defined(__clang__)
#  define PRINTF_CAST_U64(x) ((unsigned long long) (x))
#  define PRINTF_FORMAT_U64 "%llu"
#else
   /* This conversion works everywhere, but it's probably pretty slow.
    *
    * Note that 'f' takes a double vararg, not a float. */
#  define PRINTF_CAST_U64(x) ((double) (x))
#  define PRINTF_FORMAT_U64 "%.0f"
#endif


void rs_log0(int level, char const *fn, char const *fmt, ...)
    __attribute__ ((format(printf, 3, 4)));

/* In all of these, the format string is exposed as a specific
 * macro parameter so that it can be statically checked. */

#ifdef RS_ENABLE_TRACE
#  define rs_trace(fmt, ...)                            \
    do { rs_log0(RS_LOG_DEBUG, __FUNCTION__, fmt, ##__VA_ARGS);  \
    } while (0)
#else
#  define rs_trace(fmt, ...)
#endif	/* !RS_ENABLE_TRACE */

#define rs_log(l, fmt, ...) do {              \
     rs_log0((l), __FUNCTION__, (fmt) , ##__VA_ARGS__);  \
     } while (0)

#define rs_error(fmt, ...) do {                       \
     rs_log0(RS_LOG_ERR,  __FUNCTION__, (fmt), ##__VA_ARGS__);  \
     } while (0)

#define rs_fatal(fmt, ...) do {               \
     rs_log0(RS_LOG_CRIT,  __FUNCTION__, (fmt), ##__VA_ARGS__); \
     abort();                                  \
     } while (0)


void rs_trace0(char const *s, ...);
void rs_fatal0(char const *s, ...);
void rs_error0(char const *s, ...);
void rs_log0(int level, char const *fn, char const *fmt, ...);
void rs_log0_nofn(int level, char const *fmt, ...);

enum {
    RS_LOG_PRIMASK       = 7,   /**< Mask to extract priority
                                   part. \internal */

    RS_LOG_NONAME        = 8    /**< \b Don't show function name in
                                   message. */
};


/**
 * \macro rs_trace_enabled()
 *
 * Call this before putting too much effort into generating trace
 * messages.
 */

extern int rs_trace_level;

#ifdef RS_ENABLE_TRACE
#  define rs_trace_enabled() ((rs_trace_level & RS_LOG_PRIMASK) >= RS_LOG_DEBUG)
#else
#  define rs_trace_enabled() 0
#endif
