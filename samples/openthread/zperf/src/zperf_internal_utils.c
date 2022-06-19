/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/net_ip.h>

#include <stdint.h>
#include <limits.h>

#if !CONFIG_ZPERF_HACKED
#define PACKET_ID_MAX INT32_MAX
#else
#define PACKET_ID_MAX INT8_MAX
#endif


uint32_t zperf_get_next_packet_id(uint32_t current_id)
{
#if !CONFIG_ZPERF_HACKED
	int32_t next_id = current_id % ((uint32_t)PACKET_ID_MAX+1);
#else
	int8_t next_id = current_id % (PACKET_ID_MAX+1);
#endif

	if (++next_id < 0) {
		next_id = 0;
	}

	return (uint32_t)next_id;
}


uint16_t zperf_get_udp_len(struct net_udp_hdr *udp_hdr)
{
	uint16_t len = 0;

	if (udp_hdr) {
		len = ntohs(udp_hdr->len) - sizeof(*udp_hdr);
	}

	return len;
}
