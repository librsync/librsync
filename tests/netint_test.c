/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * librsync -- dynamic caching and delta update in HTTP
 *
 * Copyright (C) 2019 by Donovan Baarda <abo@minkirri.apana.org.au>
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

/* Force DEBUG on so that tests can use assert(). */
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "librsync.h"
#include "netint.h"

/* Test driver for netint. */
int main(int argc, char **argv)
{
    assert(rs_int_len((rs_long_t)0) == 1);
    assert(rs_int_len((rs_long_t)1) == 1);
    assert(rs_int_len((rs_long_t)INT8_MAX) == 1);
    assert(rs_int_len((rs_long_t)1 << 7) == 1);
    assert(rs_int_len((rs_long_t)1 << 8) == 2);
    assert(rs_int_len((rs_long_t)INT16_MAX) == 2);
#ifdef INT32_MAX
    assert(rs_int_len((rs_long_t)1 << 15) == 2);
    assert(rs_int_len((rs_long_t)1 << 16) == 4);
    assert(rs_int_len((rs_long_t)INT32_MAX) == 4);
#endif
#ifdef INT64_MAX
    assert(rs_int_len((rs_long_t)1 << 31) == 4);
    assert(rs_int_len((rs_long_t)1 << 32) == 8);
    assert(rs_int_len((rs_long_t)INT64_MAX) == 8);
#endif
    return 0;
}
