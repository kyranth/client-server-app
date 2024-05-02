#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

// Structure definition
typedef struct
{
	char server_ip_address[16]; // Assuming IPv4 address, so 15 characters + null terminator
	int tcp_pre_probing_port;
	int udp_source_port;
	int udp_destination_port;
	char udp_payload_size[10]; // Assuming payload size format, 9 characters + null terminator
	int num_udp_packets;
	int udp_packet_ttl;
	int tcp_tail_syn_port;
	int tcp_head_syn_port;
	int tcp_post_probing_port;
	char inter_measurement_time[20]; // Assuming time format, 19 characters + null terminator
} Config;

Config *createConfig();

void setConfig(const char *file, Config *config);

#endif /* CONFIG_H */
