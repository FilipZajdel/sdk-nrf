/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ZPERF_INTERNAL_H
#define __ZPERF_INTERNAL_H

#include <limits.h>
#include <net/net_ip.h>
#include <shell/shell.h>

#define IP6PREFIX_STR2(s) #s
#define IP6PREFIX_STR(p) IP6PREFIX_STR2(p)

#define MY_PREFIX_LEN 64
#define MY_PREFIX_LEN_STR IP6PREFIX_STR(MY_PREFIX_LEN)

/* Note that you can set local endpoint address in config file */
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_CONFIG_SETTINGS)
#define MY_IP6ADDR CONFIG_NET_CONFIG_MY_IPV6_ADDR
#define DST_IP6ADDR CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#define MY_IP6ADDR_SET
#else
#define MY_IP6ADDR NULL
#define DST_IP6ADDR NULL
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_CONFIG_SETTINGS)
#define MY_IP4ADDR CONFIG_NET_CONFIG_MY_IPV4_ADDR
#define DST_IP4ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define MY_IP4ADDR_SET
#else
#define MY_IP4ADDR NULL
#define DST_IP4ADDR NULL
#endif

#define PACKET_SIZE_MAX      1024
#define ZPERF_ECHO_FLAG      (1U<<1)
#define ZPERF_ACK_FLAG		 (1U<<0)
#define ZPERF_ECHO_TIMEOUT_MS (100)

#define ZPERF_CLIENT_HDR_V1_ID      (0U)
#define ZPERF_SERVER_HDR_ID	        (1U)
#define ZPERF_CLIENT_HDR_CONF_V1_ID (2U)
#define ZPERF_SERVER_HDR_ACK_ID

#if !CONFIG_ZPERF_HACKED
struct zperf_udp_datagram {
	int32_t id;
	uint32_t tv_sec;
	uint32_t tv_usec;
} __packed;

#else
struct zperf_udp_datagram {
	int8_t id;
	struct flags {
		uint8_t ack : 1;
		uint8_t echo : 1;
		uint8_t fin : 1;
		uint8_t ack_req : 1;
		uint8_t echo_req : 1;
		uint8_t fin_req : 1;
		uint8_t reserved_1 : 1;
		uint8_t reserved_2 : 1;
	} __packed flags;
} __packed;
#endif
struct zperf_client_hdr_v1 {
	int32_t flags;
	int32_t num_of_threads;
	int32_t port;
	int32_t buffer_len;
	int32_t bandwidth;
	int32_t num_of_bytes;
};

struct zperf_server_hdr {
	int32_t flags;
	int32_t total_len1;
	int32_t total_len2;
	int32_t stop_sec;
	int32_t stop_usec;
	int32_t error_cnt;
	int32_t outorder_cnt;
	int32_t datagrams;
	int32_t jitter1;
	int32_t jitter2;
};

enum zperf_test_modes {
	/* The client sends consecutive packets as fast as possible. */
	ZPERF_MODE_UNIDIRECTIONAL = 0U,

	/* The client sends consecutive packets as soon as previous are
	   akcnowledged. */
	ZPERF_MODE_ACK = 0x1U,

	/* The server responds to the client with the data of the same length as
	   received. */
	ZPERF_MODE_ECHO = 0x2U
};

struct echo_context {
    int64_t send_time;
    int32_t id_to_ack;
    struct echo_stats *echo_stats;
};

static inline uint32_t time_delta(uint32_t ts, uint32_t t)
{
	return (t >= ts) ? (t - ts) : (ULONG_MAX - ts + t);
}

int zperf_get_ipv6_addr(const struct shell *shell, char *host,
			char *prefix_str, struct in6_addr *addr);
struct sockaddr_in6 *zperf_get_sin6(void);

int zperf_get_ipv4_addr(const struct shell *shell, char *host,
			struct in_addr *addr);
struct sockaddr_in *zperf_get_sin(void);

extern void zperf_udp_upload(const struct shell *shell,
			     struct net_context *context,
			     int port,
			     unsigned int duration_in_ms,
			     unsigned int packet_size,
			     unsigned int rate_in_kbps,
			     struct zperf_results *results);

extern void zperf_udp_receiver_init(const struct shell *shell, int port);

extern void zperf_tcp_receiver_init(const struct shell *shell, int port);
extern void zperf_tcp_uploader_init(struct k_fifo *tx_queue);
extern void zperf_tcp_upload(const struct shell *shell,
			     struct net_context *net_context,
			     unsigned int duration_in_ms,
			     unsigned int packet_size,
			     struct zperf_results *results);
extern bool zperf_echo_mode_enabled(void);
extern void zperf_echo_switch(bool enabled);

extern void connect_ap(char *ssid);

const struct in_addr *zperf_get_default_if_in4_addr(void);
const struct in6_addr *zperf_get_default_if_in6_addr(void);

void zperf_tcp_stopped(void);
void zperf_tcp_started(void);

uint32_t zperf_get_next_packet_id(uint32_t current_id);
uint16_t zperf_get_udp_len(struct net_udp_hdr *udp_hdr);

#endif /* __ZPERF_INTERNAL_H */
