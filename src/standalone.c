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