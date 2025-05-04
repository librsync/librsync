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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <librsync.h>

#include "common.h"


static char in_buf[BUFFER_SIZE * 2], out_buf[BUFFER_SIZE];

static int accept_connection(void);
static int recv_signature(int sock, rs_signature_t **sig);
static int send_delta(int sock, rs_signature_t *sig, const char *fname);

int main(int argc, char *argv[])
{
    /* Parse arguments (use stdin if no argument) */
    const char *fname = (argc >= 2) ? argv[1] : NULL;

    puts("Waiting for connection...");
    int sock = accept_connection();
    if (sock == -1) {
        return EXIT_FAILURE;
    }

    puts("Receiving signature...");
    rs_signature_t *sig;
    int ret = recv_signature(sock, &sig);
    if (ret == -1) {
        close(sock);
        return EXIT_FAILURE;
    }

    puts("Sending delta...");
    ret = send_delta(sock, sig, fname);
    rs_free_sumset(sig);
    if (ret == -1) {
        close(sock);
        return EXIT_FAILURE;
    }

    close(sock);
    puts("Success!");
    return EXIT_SUCCESS;
}

static int accept_connection(void) {
    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        return -1;
    }

    /* Enable reuse address (to avoid "Address already in use" errors */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Assign IP address and port */
    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    /* Bind socket to given IP address */
    int ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("Failed to bind socket");
        close(sock);
        return -1;
    }

    /* Listen for incoming connections. In a real-world application you should
       have a larger "connection request" queue. */
    ret = listen(sock, 1);
    if (ret == -1) {
        perror("Failed to listen");
        close(sock);
        return -1;
    }

    /* Accept incoming connection */
    struct sockaddr_in client_addr; socklen_t addr_len;
    int conn = accept(sock, (struct sockaddr *)&client_addr, &addr_len);

    /* We don't expect any more connections in this example. In a real-world
       application you should keep this socket open to accept more
       connections. */
    close(sock);

    if (conn == -1) {
        perror("Failed to accept");
        return -1;
    }

    return conn;
}

static int recv_signature(int sock, rs_signature_t **sig) {
    rs_job_t *job = rs_loadsig_begin(sig);
    assert(job != NULL);

    /* Setup buffers */
    rs_buffers_t bufs = { 0 };

    rs_result res;
    do {
        if ((bufs.eof_in == 0) && (bufs.avail_in <= BUFFER_SIZE)) {
            if (bufs.avail_in > 0) {
                /* Leftover tail data, move it to front */
                memmove(in_buf, bufs.next_in, bufs.avail_in);
            }

            size_t n_bytes;
            int ret = recv_message(sock, in_buf + bufs.avail_in, &n_bytes, &bufs.eof_in);
            if (ret == -1) {
                rs_job_free(job);
                return -1;
            }

            bufs.next_in = in_buf;
            bufs.avail_in += n_bytes;
        }

        res = rs_job_iter(job, &bufs);
        if (res != RS_DONE && res != RS_BLOCKED) {
            rs_job_free(job);
            return -1;
        }

        /* The job should take care of draining the buffers */
    } while (res != RS_DONE);

    rs_job_free(job);
    return 0;
}

static int send_delta(int sock, rs_signature_t *sig, const char *fname) {
    /* Build hash table */
    rs_result res = rs_build_hash_table(sig);
    if (res != RS_DONE) {
        return -1;
    }

    /* Open file up-to-date file */
    FILE *file = rs_file_open(fname, "rb", 0);
    assert(file != NULL);

    /* Start generating delta */
    rs_job_t *job = rs_delta_begin(sig);
    assert(job != NULL);

    /* Setup buffers */
    rs_buffers_t bufs = { 0 };
    bufs.next_out = out_buf;
    bufs.avail_out = BUFFER_SIZE; /* We cannot send more in one message */

    do {
        if ((bufs.eof_in == 0) && (bufs.avail_in < sizeof(in_buf))) {
            if (bufs.avail_in > 0) {
                /* Left over tail data, move to front */
                memmove(in_buf, bufs.next_in, bufs.avail_in);
            }

            /* Fill input buffer */
            size_t n_bytes = fread(in_buf + bufs.avail_in, 1, sizeof(in_buf) - bufs.avail_in, file);
            if (n_bytes == 0) {
                if (ferror(file)) {
                    perror("Failed to read file");
                    rs_file_close(file);
                    rs_job_free(job);
                    return -1;
                }
                bufs.eof_in = feof(file);
            }

            bufs.next_in = in_buf;
            bufs.avail_in += n_bytes;
        }

        res = rs_job_iter(job, &bufs);
        if (res != RS_DONE && res != RS_BLOCKED) {
            rs_file_close(file);
            rs_job_free(job);
            return -1;
        }

        /* Drain output buffer, if there is data */
        assert(bufs.next_out >= out_buf);
        size_t present = (size_t)(bufs.next_out - out_buf);
        if (present > 0 || res == RS_DONE) {
            assert(present <= BUFFER_SIZE);
            int ret = send_message(sock, out_buf, present, (res == RS_DONE) ? 1 : 0);
            if (ret == -1) {
                rs_file_close(file);
                rs_job_free(job);
                return -1;
            }

            bufs.next_out = out_buf;
            bufs.avail_out = BUFFER_SIZE;
        }
    } while (res != RS_DONE);

    rs_file_close(file);
    rs_job_free(job);
    return 0;
}
