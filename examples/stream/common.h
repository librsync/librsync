/*= -*- c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *
 * librsync -- the library for network deltas
 *
 * Copyright (C) 2024 by Lars Erik Wik <lars.erik.wik@northern.tech>
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

/** \file common.h
 * Simple network protocol on top of TCP. Used for client-server communication
 * in the Stream API example code.
 *
 * Header format:
 *   +----------+----------+----------|
 *   | SDU Len. | Reserved | EoF Flag |
 *   +----------+----------+----------+
 *   | 12 bits  | 3 bits   | 1 bit    |
 *   +----------+----------+----------|
 *
 * The header fields are defined as follows:
 * SDU Length           Length of the SDU (i.e. payload) encapsulated within
 *                      this datagram.
 * Reserved             3 bits reserved for future use.
 * End-of-File Flag     Determines whether or not the receiver should expect
 *                      to receive more datagrams.
 * */
#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <librsync.h>

/** The port the server will be listening on */
#define PORT 5612

/** It's the largest value we can represent with the 12 bits in the SDU Length
 * header field (2^12-1 = 4095). Hence, it's the largest allowed payload.
 */
#define BUFFER_SIZE 4095


static int send_message(int sock, const char *msg, size_t len, int eof) {
    assert(len <= BUFFER_SIZE);

    uint16_t header = (uint16_t)len << 4;

    /* Set EoF flag */
    if (eof != 0) {
        header |= 1;
    }

    /* Send header */
    header = htons(header);
    size_t n_bytes = 0;
    do {
        ssize_t ret = write(sock, &header + n_bytes, sizeof(header) - n_bytes);
        if (ret < 0) {
            if (errno == EINTR) {
                /* The call was interrupted by a signal before any data was
                   written. It is safe to continue. */
                continue;
            }
            perror("Failed to send message header");
            return -1;
        }
        n_bytes += (size_t)ret;
    } while (n_bytes < sizeof(header));

    if (len > 0) {
        /* Send payload */
        n_bytes = 0;
        do {
            ssize_t ret = write(sock, msg + n_bytes, len - n_bytes);
            if (ret < 0) {
                if (errno == EINTR) {
                    /* The call was interrupted by a signal before any data was
                       written. It is safe to continue. */
                    continue;
                }
                perror("Failed to send message payload");
                return -1;
            }
            n_bytes += (size_t)ret;
        } while (n_bytes < len);
    }

    return 0;
}

static int recv_message(int sock, char *msg, size_t *len, int *eof) {
    /* Receive header */
    uint16_t header;
    size_t n_bytes = 0;
    do {
        ssize_t ret = read(sock, &header + n_bytes, sizeof(header) - n_bytes);
        if (ret < 0) {
            if (errno == EINTR) {
                /* The call was interrupted by a signal before any data was
                   read. It is safe to continue. */
                continue;
            }
            perror("Failed to receive message header");
            return -1;
        }
        n_bytes += (size_t)ret;
    } while (n_bytes < sizeof(header));
    header = ntohs(header);

    /* Extract EOF flag */
    *eof = header & 1;

    /* Extract message length */
    *len = header >> 4;

    if (*len > 0) {
        /* Read payload */
        n_bytes = 0;
        do {
            ssize_t ret = read(sock, msg + n_bytes, *len - n_bytes);
            if (ret < 0) {
                if (errno == EINTR) {
                    /* The call was interrupted by a signal before any data was
                       read. It is safe to continue. */
                    continue;
                }
                perror("Failed to receive message payload");
                return -1;
            }
            n_bytes += (size_t)ret;
        } while (n_bytes < *len);
    }

    return 0;
}

#endif                          /* !COMMON_H */
