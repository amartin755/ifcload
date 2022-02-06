// SPDX-License-Identifier: GPL-3.0-only
/*
 * NETNAG <https://github.com/amartin755/netnag>
 * Copyright (C) 2022 Andreas Martin (netnag@mailbox.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include "netdev.h"

uint64_t rx_bytes_prev = 0;
uint64_t tx_bytes_prev = 0;
uint64_t rx_packets_prev = 0;
uint64_t tx_packets_prev = 0;
uint64_t rx_bytes = 0;
uint64_t tx_bytes = 0;
uint64_t rx_packets = 0;
uint64_t tx_packets = 0;
struct timespec t_prev;
struct timespec t;

static unsigned updateTime = 500000;


static double convert_to_human_readable (int binary, uint64_t value, char** prefix)
{
    if (!binary)
    {
        if (value > 1000000000)
        {
            *prefix = "G";
            return value / 1000000000.0;
        }
        if (value > 1000000)
        {
            *prefix = "M";
            return value / 1000000.0;
        }
        if (value > 1000)
        {
            *prefix = "k";
            return value / 1000.0;
        }
    }
    else
    {
        if (value > 1024*1024*1024)
        {
            *prefix = "Gi";
            return value / (double)(1024*1024*1024);
        }
        if (value > 1024*1024)
        {
            *prefix = "Mi";
            return value / (double)(1024*1024);
        }
        if (value > 1024)
        {
            *prefix = "Ki";
            return value / 1024.0;
        }
    }
    *prefix = " ";
    return (double)value;
}

static void print_stats (void)
{
    uint64_t rx_diff = rx_bytes - rx_bytes_prev;
    uint64_t tx_diff = tx_bytes - tx_bytes_prev;
    uint64_t rx_pack_diff = rx_packets - rx_packets_prev;
    uint64_t tx_pack_diff = tx_packets - tx_packets_prev;

    struct timespec t_diff;
    t_diff.tv_sec  = t.tv_sec  - t_prev.tv_sec;
    t_diff.tv_nsec = t.tv_nsec - t_prev.tv_nsec;
    if (t_diff.tv_nsec < 0)
    {
        t_diff.tv_sec--;
        t_diff.tv_nsec += 1000000000L;
    }

    uint64_t diff_ms = t_diff.tv_sec * 1000 + t_diff.tv_nsec / 1000000L;
    uint64_t tx_bandwidth = tx_diff * 8 * 1000 / diff_ms;
    uint64_t rx_bandwidth = rx_diff * 8 * 1000 / diff_ms;
    uint64_t tx_packets_per_sec = tx_pack_diff * 1000 / diff_ms;
    uint64_t rx_packets_per_sec = rx_pack_diff * 1000 / diff_ms;


    char *tx_bw_prefix, *rx_bw_prefix;
    double tx_bw = convert_to_human_readable (0, tx_bandwidth, &tx_bw_prefix);
    double rx_bw = convert_to_human_readable (0, rx_bandwidth, &rx_bw_prefix);
    char *tx_bytes_prefix, *rx_bytes_prefix;
    double tx_bytes_h = convert_to_human_readable (1, tx_bytes, &tx_bytes_prefix);
    double rx_bytes_h = convert_to_human_readable (1, rx_bytes, &rx_bytes_prefix);

    const char ASCII_ESC = 27;
    printf( "%c[2J", ASCII_ESC );
    printf( "%c[H", ASCII_ESC );

    printf("RX: ");
    printf("%*"PRIu64" packets", 10, rx_packets);
    printf("%*"PRIu64" packets/s", 10, rx_packets_per_sec);
    printf("%*.3f %sByte", 10, rx_bytes_h, rx_bytes_prefix);
    printf("%*.2f %sBit/s", 10, rx_bw, rx_bw_prefix);
    printf("\n");
    printf("TX: ");
    printf("%*"PRIu64" packets", 10, tx_packets);
    printf("%*"PRIu64" packets/s", 10, tx_packets_per_sec);
    printf("%*.3f %sByte", 10, tx_bytes_h, tx_bytes_prefix);
    printf("%*.2f %sBit/s", 10, tx_bw, tx_bw_prefix);
    printf("\n");
}

static volatile sig_atomic_t sigIntCached;
static void sigintHandler (int signal)
{
    if (signal == SIGINT)
    {
        sigIntCached++;
    }
}

static void print_usage (const char* arg)
{
    fprintf(stderr, "Usage: %s [-t cs] device\n", arg);
}

int main(int argc, char** argv)
{
    int opt;

    if (argc < 2)
    {
        print_usage (argv[0]);
        return -1;
    }

    while ((opt = getopt(argc, argv, "t:")) != -1)
    {
        if (opt == 't')
        {
            int cs = atoi(optarg);
            if (cs < 1 || cs > 36000)
            {
                fprintf (stderr, "Allowed range for opion -t  is 1 - 36000\n");
                return 1;
            }
            updateTime = cs * 100* 1000;

        }
        else
        {
            print_usage (argv[0]);
            return 1;
        }
    }


    if (net_open ())
        return -2;

    signal (SIGINT, sigintHandler);


    while (!sigIntCached)
    {
        if (net_get_stats (argv[optind], &t, &rx_bytes, &tx_bytes, &rx_packets, &tx_packets))
            return -3;

        print_stats ();

        rx_bytes_prev = rx_bytes;
        tx_bytes_prev = tx_bytes;
        rx_packets_prev = rx_packets;
        tx_packets_prev = tx_packets;
        t_prev = t;

        usleep(updateTime);
    }

    printf("\n");
    net_close ();
    return 0;
}

