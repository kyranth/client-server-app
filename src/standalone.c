#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/cJSON.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/time.h>

// Structure definition
typedef struct
{
	char *server_ip_address;
	int udp_source_port;
	int udp_destination_port;
	int tcp_head_syn_port;
	int tcp_tail_syn_port;
	int tcp_pre_probing_port;
	int tcp_post_probing_port;
	int udp_payload_size;
	int inter_measurement_time;
	int num_udp_packets;
	int udp_packet_ttl;
} Config;

Config *createConfig()
{
	Config *config = (Config *)malloc(sizeof(Config));
	if (config == NULL)
	{
		perror("Failed to allocate memory for Config");
		exit(1);
	}

	// Assuming IPv4 address, so 15 characters + null terminator
	config->server_ip_address = malloc(16 * sizeof(char));

	if (config->server_ip_address == NULL)
	{
		perror("Failed to allocate memory for server_ip_address");
		exit(1);
	}

	return config;
}

void p_error(const char *msg)
{
	perror(msg);
	exit(1);
}

/**
 * @brief Takes a filename and returns the cJSON object
 *
 * @param file pointer to a filename
 * @param config structure
 */
void setConfig(const char *file, Config *config)
{
	// open the file
	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		p_error("Could not open config file\n");
	}

	// read the file contents into a string
	char buffer[1024];
	fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	// parse the JSON data
	cJSON *json = cJSON_Parse(buffer);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			printf("Error: %s\n", error_ptr);
		}
		exit(1);
	}
	else
	{
		strcpy(config->server_ip_address, cJSON_GetObjectItemCaseSensitive(json, "server_ip_address")->valuestring);
		config->udp_source_port = cJSON_GetObjectItemCaseSensitive(json, "udp_source_port")->valueint;
		config->udp_destination_port = cJSON_GetObjectItemCaseSensitive(json, "udp_destination_port")->valueint;
		config->tcp_head_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_head_syn_port")->valueint;
		config->tcp_tail_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_tail_syn_port")->valueint;
		config->tcp_pre_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valueint;
		config->tcp_post_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_post_probing_port")->valueint;
		config->udp_payload_size = cJSON_GetObjectItemCaseSensitive(json, "udp_payload_size")->valueint;
		config->inter_measurement_time = cJSON_GetObjectItemCaseSensitive(json, "inter_measurement_time")->valueint;
		config->num_udp_packets = cJSON_GetObjectItemCaseSensitive(json, "num_udp_packets")->valueint;
		config->udp_packet_ttl = cJSON_GetObjectItemCaseSensitive(json, "udp_packet_ttl")->valueint;
	}

	// delete the JSON object
	cJSON_Delete(json);
}

/**
 * @brief Takes udphdr pointer and Config file to construct UDP Header
 *
 * @param udp_header pinter to struct
 * @param config configuration file
 */
struct udphdr *fill_udp_header(struct udphdr *udp_header, Config *config)
{
	udp_header->source = htons(config->udp_source_port);
	udp_header->dest = htons(config->udp_destination_port);					   // TODO uint16_t
	udp_header->len = htons(sizeof(struct udphdr) + config->udp_payload_size); // TODO uint16_t
	udp_header->check = 0;													   // Check calculation
}

/**
 * @brief Contructs the IP Header
 *
 * @param ip_header iphdr pointer
 * @param config configuration file
 * @param cliaddr struct
 */
void fill_ip_header(struct iphdr *ip_header, Config *config, struct sockaddr_in cliaddr)
{
	ip_header->ihl = 5;
	ip_header->version = 4;
	ip_header->tos = 0;
	ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + config->udp_payload_size);
	ip_header->id = htons(0);				 // Can be any value
	ip_header->frag_off = htons(0x4000);	 // Don't fragment flag
	ip_header->ttl = config->udp_packet_ttl; // Time to live
	ip_header->protocol = IPPROTO_UDP;
	ip_header->check = 0;						// Checksum calculation is optional
	ip_header->saddr = cliaddr.sin_addr.s_addr; // TODO what is this?
	ip_header->daddr = inet_addr(config->server_ip_address);
}

/**
 * @brief Send UDP Packet
 *
 * @param sockfd socket file descriptor
 * @param config file struct
 * @param servaddr address struct
 */
void send_udp_packet(int sockfd, Config *config, struct sockaddr_in servaddr, char *source_ip, char *dest_ip, uint16_t source_port, uint16_t dest_port, uint16_t packet_id, char *payload, uint16_t payload_size)
{
	char buffer[PACKET_SIZE];
	struct iphdr *ip_header = (struct iphdr *)buffer;
	struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct iphdr));
	char *packet_payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

	// Fill IP header
	fill_ip_header(ip_header, config, servaddr);

	// Fill UDP header
	fill_udp_header(udp_header, config);

	// Fill packet payload
	*(uint16_t *)packet_payload = htons(packet_id);
	memcpy(packet_payload + sizeof(uint16_t), payload, payload_size - sizeof(uint16_t));

	// Calculate IP checksum
	ip_header->check = checksum((unsigned short *)buffer, sizeof(struct iphdr));

	// Send packet
	sendto(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
}