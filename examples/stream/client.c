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
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include <librsync.h>

#include "common.h"

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define IP_ADDRESS "127.0.0.1"


static char in_buf[BUFFER_SIZE * 2], out_buf[BUFFER_SIZE];

static int connect_to_server(const char *ip_addr);
static int send_signature(int sock, const char *fname);
static int recv_delta_and_patch_file(int sock, const char *fname);

int main(int argc, char *argv[]) {
    /* Parse arguments (use stdin and stdout if no argument) */
    const char *fname = (argc >= 2) ? argv[1] : NULL;

    puts("Connecting to server...");
    int sock = connect_to_server(IP_ADDRESS);
    if (sock == -1) {
        return EXIT_FAILURE;
    }

    puts("Sending signature...");
    int ret = send_signature(sock, fname);
    if (ret == -1) {
        close(sock);
        return EXIT_FAILURE;
    }

    puts("Receiving delta and patching file...");
    ret = recv_delta_and_patch_file(sock, fname);
    if (ret == -1) {
        close(sock);
        return EXIT_FAILURE;
    }

    close(sock);
    puts("Success!");
    return EXIT_SUCCESS;
}

static int connect_to_server(const char *ip_addr) {
    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        return -1;
    }

    /* Assign IP address and port */
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(PORT);

    /* Connect to server */
    int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("Failed to connect");
        close(sock);
        return -1;
    }

    return sock;
}

static int send_signature(int sock, const char *fname) {
    /* Make sure the basis file exists, unless it is stdin */
    const int use_io_stream = (fname == NULL) || (strcmp(fname, "-") == 0);
    if (!use_io_stream) {
        int fd = open(fname, O_RDONLY | O_CREAT);
        if (fd != -1) {
            close(fd);
        }
    }

    /* Open basis file */
    FILE *file = rs_file_open(fname, "rb", 0);
    assert(file != NULL);

    /* Get file size */
    rs_long_t fsize = rs_file_size(file);

    /* Get recommended arguments */
    rs_magic_number sig_magic = 0;
    size_t block_len = 0, strong_len = 0;
    rs_result res = rs_sig_args(fsize, &sig_magic, &block_len, &strong_len);
    if (res != RS_DONE) {
        rs_file_close(file);
        return -1;
    }

    /* Start generating signature */
    rs_job_t *job = rs_sig_begin(block_len, strong_len, sig_magic);
    assert(job != NULL);

    /* Setup buffers */
    rs_buffers_t bufs = { 0 };
    bufs.next_out = out_buf;
    bufs.avail_out = BUFFER_SIZE; /* We cannot send more in one message */

    size_t tot_bytes_sent = 0;

    /* Generate signature */
    do {
        if ((bufs.eof_in == 0) && (bufs.avail_in < sizeof(in_buf))) {
            if (bufs.avail_in > 0) {
                /* Leftover tail data, move it to front */
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

                /* End-of-File reached */
                bufs.eof_in = feof(file);
                assert(bufs.eof_in != 0);
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

        assert(bufs.next_out >= out_buf);
        size_t present = (size_t)(bufs.next_out - out_buf);
        if (present > 0 || res == RS_DONE) {
            /* Drain output buffer */
            assert(present <= BUFFER_SIZE);
            int ret = send_message(sock, out_buf, present, (res == RS_DONE) ? 1 : 0);
            if (ret == -1) {
                rs_file_close(file);
                rs_job_free(job);
                return -1;
            }

            tot_bytes_sent += present;
            bufs.next_out = out_buf;
            bufs.avail_out = BUFFER_SIZE;
        }
    } while (res != RS_DONE);

    rs_file_close(file);
    rs_job_free(job);

    printf("Sent %zu bytes\n", tot_bytes_sent);

    return 0;
}

static int recv_delta_and_patch_file(int sock, const char *fname_old) {
    const int use_io_stream = (fname_old == NULL) || (strcmp(fname_old, "-") == 0);

    const char *fname_new = NULL;
    char path[PATH_MAX];
    if (!use_io_stream) {
        int ret = snprintf(path, sizeof(path), "%s.new", fname_old);
        if (ret < 0 || (size_t)ret >= sizeof(path)) {
            fputs("Filename too long\n", stderr);
            return -1;
        }
        fname_new = path;
    }

    /* Open new file */
    FILE *new = rs_file_open(fname_new, "wb", 1);
    assert(new != NULL);

    /* Open basis file */
    FILE *old = old = rs_file_open(fname_old, "rb", 0);
    assert(old != NULL);

    rs_job_t *job = rs_patch_begin(rs_file_copy_cb, old);
    assert(job != NULL);

    /* Setup buffers */
    rs_buffers_t bufs = { 0 };
    bufs.next_out = out_buf;
    bufs.avail_out = sizeof(out_buf);

    size_t tot_bytes_received = 0;

    rs_result res;
    do {
        if ((bufs.eof_in == 0) && (bufs.avail_in < BUFFER_SIZE)) {
            if (bufs.avail_in > 0) {
                /* Left over tail data, move to front */
                memmove(in_buf, bufs.next_in, bufs.avail_in);
            }

            size_t n_bytes;
            int ret = recv_message(sock, in_buf + bufs.avail_in, &n_bytes, &bufs.eof_in);
            if (ret == -1) {
                rs_file_close(new);
                rs_file_close(old);
                rs_job_free(job);
                return -1;
            }

            tot_bytes_received += n_bytes;
            bufs.next_in = in_buf;
            bufs.avail_in += n_bytes;
        }

        res = rs_job_iter(job, &bufs);
        if (res != RS_DONE && res != RS_BLOCKED) {
            rs_file_close(new);
            rs_file_close(old);
            rs_job_free(job);
            return -1;
        }

        /* Drain output buffer, if there is data */
        assert(bufs.next_out >= out_buf);
        size_t present = (size_t)(bufs.next_out - out_buf);
        if (present > 0) {
            size_t n_bytes = fwrite(out_buf, 1, present, new);
            if (n_bytes == 0) {
                perror("Failed to write to file");
                rs_file_close(new);
                rs_file_close(old);
                rs_job_free(job);
                return -1;
            }

            bufs.next_out = out_buf;
            bufs.avail_out = sizeof(out_buf);
        }
    } while (res != RS_DONE);

    rs_file_close(new);
    rs_file_close(old);
    rs_job_free(job);

    printf("Received %zu bytes\n", tot_bytes_received);

    if (!use_io_stream) {
        int ret = rename(fname_new, fname_old);
        if (ret == -1) {
            perror("Failed to swap basis file with patched file");
            return -1;
        }
    }

    return 0;
}
