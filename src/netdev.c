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


#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#include "netdev.h"

static struct nl_sock *s;

int net_open (void)
{
    s = nl_socket_alloc();
    if (s == NULL)
        return 1;
    return nl_connect(s, NETLINK_ROUTE);
}

void net_close (void)
{
    nl_socket_free(s);
}

int net_get_stats (const char* device, struct timespec *t, uint64_t *rx_bytes, uint64_t *tx_bytes, uint64_t *rx_packets, uint64_t *tx_packets)
{
    struct rtnl_link *link;
    if (!rtnl_link_get_kernel(s, 0, device, &link))
    {
        clock_gettime (CLOCK_MONOTONIC_COARSE, t);
        *rx_bytes   = rtnl_link_get_stat (link, RTNL_LINK_RX_BYTES);
        *tx_bytes   = rtnl_link_get_stat (link, RTNL_LINK_TX_BYTES);
        *rx_packets = rtnl_link_get_stat (link, RTNL_LINK_RX_PACKETS);
        *tx_packets = rtnl_link_get_stat (link, RTNL_LINK_TX_PACKETS);
        rtnl_link_put(link);

        return 0;
    }
    return -1;
}