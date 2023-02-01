/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * rabinkarp_perf -- peformance tests for the rabinkarp checksum.
 *
 * Copyright (C) 2003 by Donovan Baarda <abo@minkirri.apana.org.au>
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

#include <stdio.h>
#include <inttypes.h>
#include "rabinkarp.h"

int main(int argc, char **argv)
{
    rabinkarp_t r;
    int i;
    uint8_t buf[1024];
    uint32_t sum;

    rabinkarp_init(&r);
    for (i = 0; i < 1024 * 1024; i++) {
        fread(buf, 1024, 1, stdin);
        rabinkarp_update(&r, buf, 1024);
    }
    sum = rabinkarp_digest(&r);
    printf("%08" PRIx32 "\n", sum);
    return 0;
}
