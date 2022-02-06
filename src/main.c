// SPDX-License-Identifier: GPL-3.0-only
/*
 * TCPPUMP <https://github.com/amartin755/netnag>
 * Copyright (C) 2020 Andreas Martin (netnag@mailbox.org)
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
#include <inttypes.h>

#include <unistd.h>
#include <signal.h>

#include "netdev.h"

uint64_t rx_bytes_prev = 0;
uint64_t tx_bytes_prev = 0;
uint64_t rx_bytes = 0;
uint64_t tx_bytes = 0;
struct timespec t_prev;
struct timespec t;

static int updateTime = 500000;


static double convert_to_human_readable (uint64_t value, char** prefix)
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
    *prefix = " ";
    return (double)value;
}

static void print_stats (void)
{
    uint64_t rx_diff = rx_bytes - rx_bytes_prev;
    uint64_t tx_diff = tx_bytes - tx_bytes_prev;

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


    char *tx_prefix, *rx_prefix;
    double tx_bw = convert_to_human_readable (tx_bandwidth, &tx_prefix);
    double rx_bw = convert_to_human_readable (rx_bandwidth, &rx_prefix);;



//    printf("rx_byte=%" PRIu64 ", tx_bytes=%" PRIu64 ", rx_bandwidth=%" PRIu64", tx_bandwidth=%" PRIu64 "\n", rx_bytes, tx_bytes, rx_bandwidth, tx_bandwidth);
//    printf("rx_byte=%" PRIu64 ", tx_bytes=%" PRIu64 ", rx_bandwidth=%.2f %sBit/s , tx_bandwidth=%.2f %sBit/s\n", rx_bytes, tx_bytes, rx_bw, rx_prefix, tx_bw, tx_prefix);
    printf("RX: %*.2f %sBit/s , TX: %*.2f %sBit/s\r", 10, rx_bw, rx_prefix, 10, tx_bw, tx_prefix);
    fflush(stdout);
}

static volatile sig_atomic_t sigIntCached;
static void sigintHandler (int signal)
{
    if (signal == SIGINT)
    {
        sigIntCached++;
    }
}


int main(int argc, char** argv)
{

    if (argc < 2)
        return -1;

    if (net_open ())
        return -2;

    signal (SIGINT, sigintHandler);


    while (!sigIntCached) 
    {
        if (net_get_stats (argv[1], &t, &rx_bytes, &tx_bytes))
            return -3;

        print_stats ();

        rx_bytes_prev = rx_bytes;
        tx_bytes_prev = tx_bytes;
        t_prev = t;

        usleep(updateTime);
    }

    printf("\n");
    net_close ();
    return 0;
}

