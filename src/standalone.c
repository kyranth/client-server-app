#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "../lib/cJSON.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/time.h>

#define OPT_SIZE 20

// Function prototype for the thread function
void *recv_rst_packet(void *arg);

// Structure definition
typedef struct
{
	char *server_ip_address;
	char *client_ip_address;
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

// pseudo header needed for tcp header checksum calculation
struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t tcp_length;
};

/**
 * @brief Creates and allocates memory to the config structure.
 *
 * @return pointer to config struture
 */
Config *createConfig()
{
	Config *config = (Config *)malloc(sizeof(Config));
	if (config == NULL)
	{
		perror("Failed to allocate memory for Config");
		exit(1);
	}

	// Assuming IPv4 address, so 15 characters + null terminator
	config->server_ip_address = (char *)malloc(16 * sizeof(char));
	config->client_ip_address = (char *)malloc(16 * sizeof(char));

	if (config->server_ip_address == NULL)
	{
		perror("Failed to allocate memory for server_ip_address");
		exit(1);
	}

	return config;
}

/**
 * @brief Prints error message and terminates program
 */
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
		strcpy(config->client_ip_address, cJSON_GetObjectItemCaseSensitive(json, "client_ip_address")->valuestring);
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
 * Creates a UDP file descriptor and returns for use.
 *
 * @return sockfd file descriptor or -1 if unsuccessful
 */
int init_udp()
{
	int sockfd;
	// Creating UDP socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
	{
		printf("ERROR: UDP Socket creation failed\n");
		return -1;
	}
	return sockfd;
}

unsigned short checksum(const char *buf, unsigned size)
{
	unsigned sum = 0, i;

	/* Accumulate checksum */
	for (i = 0; i < size - 1; i += 2)
	{
		unsigned short word16 = *(unsigned short *)&buf[i];
		sum += word16;
	}

	/* Handle odd-sized case */
	if (size & 1)
	{
		unsigned short word16 = (unsigned char)buf[i];
		sum += word16;
	}

	/* Fold to get the ones-complement result */
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	/* Invert to get the negative in ones-complement arithmetic */
	return ~sum;
}

void create_syn_packet(struct sockaddr_in *src, struct sockaddr_in *dst, char **out_packet, int *out_packet_len)
{
	/* Datagram to represent the packet this buffer will contain ip header, tcp header,
	and payload. we'll point an ip header structure
	at its beginning, and a tcp header structure after
	that to write the header values into it */
	char datagram[4096];

	// required structs for IP and TCP header
	struct iphdr *iph = (struct iphdr *)datagram;
	struct tcphdr *tcph = (struct tcphdr *)(datagram + sizeof(struct tcphdr));
	struct pseudo_header psh;

	// IP header configuration
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
	iph->id = htonl(rand() % 65535); // id of this packet
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_TCP;
	iph->check = 0; // correct calculation follows later
	iph->saddr = src->sin_addr.s_addr;
	iph->daddr = dst->sin_addr.s_addr;

	// TCP header configuration
	tcph->source = src->sin_port;
	tcph->dest = dst->sin_port;
	tcph->seq = htonl(rand() % 4294967295);
	tcph->ack_seq = htonl(0);
	tcph->doff = 10; // tcp header size
	tcph->fin = 0;
	tcph->syn = 1;
	tcph->rst = 0;
	tcph->psh = 0;
	tcph->ack = 0;
	tcph->urg = 0;
	tcph->check = 0;			// correct calculation follows later
	tcph->window = htons(5840); // window size
	tcph->urg_ptr = 0;

	// TCP pseudo header for checksum calculation
	psh.source_address = src->sin_addr.s_addr;
	psh.dest_address = dst->sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);
	int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;

	// fill pseudo packet
	char *pseudogram = malloc(psize);
	memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + OPT_SIZE);

	// TCP options are only set in the SYN packet
	// ---- set mss ----
	datagram[40] = 0x02;
	datagram[41] = 0x04;
	int16_t mss = htons(48); // mss value
	memcpy(datagram + 42, &mss, sizeof(int16_t));

	// ---- enable SACK ----
	datagram[44] = 0x04;
	datagram[45] = 0x02;

	// do the same for the pseudo header
	pseudogram[32] = 0x02;
	pseudogram[33] = 0x04;
	memcpy(pseudogram + 34, &mss, sizeof(int16_t));
	pseudogram[36] = 0x04;
	pseudogram[37] = 0x02;

	tcph->check = checksum((const char *)pseudogram, psize);
	iph->check = checksum((const char *)datagram, iph->tot_len);

	*out_packet = datagram;
	*out_packet_len = iph->tot_len;
	free(pseudogram);
}

void send_udp_packets(Config *config)
{
	int sockfd = init_udp();

	struct sockaddr_in cliaddr, servaddr;
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(config->udp_source_port);

	servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
	servaddr.sin_port = htons(config->udp_destination_port);
	printf("UDP Connection with (%s/%d)...\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

	// UDP Bind
	if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
	{
		p_error("ERROR: Bind failed\n");
	}

	// variable to hold num packets and payload size from config file
	int num_packets = config->num_udp_packets;
	size_t payload_size = (size_t)(config->udp_payload_size - sizeof(uint16_t));

	// UDP packet structure
	typedef struct
	{
		unsigned short packet_id;
		char payload[payload_size - 2];
	} UDP_Packet;

	UDP_Packet low_packet;

	// Generate low entropy payload data (all 0s)
	memset(low_packet.payload, 0, payload_size);

	// Send low entropy data packet
	printf("Sending Packet Train...\n");
	for (int i = 0; i < num_packets; ++i)
	{
		// Prepare packet payload with packet ID
		low_packet.packet_id = (unsigned short)i;

		// Send
		sendto(sockfd, &low_packet, sizeof(low_packet.payload), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

		// Wait to prevent sending packets too fast
		usleep(200);
	}
}

typedef struct
{
	int sockfd;
	struct sockaddr_in listenaddr;
	struct timeval *delta;
} recv_config;

void *recv_rst_packet(void *arg)
{
	recv_config *settings = (recv_config *)arg;

	char buffer[1048];

	// Time variables
	struct timeval first, last;

	// Listen for RST packets
	gettimeofday(&first, NULL);

	// Recv variables
	int rst_packets_received = 0;
	while (rst_packets_received < 2)
	{
		int len = sizeof(settings->listenaddr);
		if (recvfrom(settings->sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&settings->listenaddr, &len) < 0)
		{
			printf("ERROR: recvfrom() failed\n");
			break;
		}
		else
		{
			struct iphdr *ip = (struct iphdr *)buffer;
			struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct iphdr));
			if (tcp->rst)
			{
				printf("Received RST packet\n");
				rst_packets_received++;
			}
		}
	}
	gettimeofday(&last, NULL);
	settings->delta->tv_sec = last.tv_sec - first.tv_sec;
}

int main(int argc, char *argv[])
{
	/** Check for command line arguments */
	if (argc < 1)
	{
		printf("Error: Expected (1) argument config.json file\n");
		exit(0);
	}
	char *config_file = argv[1]; // Get config file from command line

	// Initialize a config structure to NULL values
	Config *config = createConfig();

	/** Read the config file for server IP and fill it with JSON data */
	setConfig(config_file, config);

	/* <------------- Socket Setup -------------> */
	int sockfd;

	// Create socket
	if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
	{
		p_error("ERROR: Failed to create socket\n");
	}

	// Set socket options for RAW Sockets
	const int on = 1;
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		p_error("ERROR: Failed to set socket options\n");
	}

	// Set timeout
	struct timeval timeout;
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
	{
		p_error("ERROR: setsockopt timeout failed\n");
	}

	/* <------------- End of socket Setup -------------> */

	// Set server address
	struct sockaddr_in servaddr;
	servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
	servaddr.sin_port = htons(config->tcp_head_syn_port);

	// Set client address
	struct sockaddr_in cliaddr;
	cliaddr.sin_addr.s_addr = inet_addr(config->client_ip_address);
	cliaddr.sin_port = htons(config->tcp_head_syn_port);

	char *packet;
	int packet_len;

	// Create SYN packet
	create_syn_packet(&cliaddr, &servaddr, &packet, &packet_len);

	// Send head SYN packet to port X
	if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) == -1)
	{
		p_error("ERROR: Send failed\n");
	}
	else
	{
		printf("Successfully sent SYN packet!\n");
	}

	// Multi-threaded listen and send packets
	struct timeval delta;
	pthread_t listen;

	recv_config *listen_addr;
	listen_addr->sockfd = sockfd;
	listen_addr->listenaddr = cliaddr;
	listen_addr->delta = &delta;

	// Listen for rst packets on different thread
	pthread_create(&listen, NULL, recv_rst_packet, listen_addr);

	// Send udp in main thread
	send_udp_packets(config);

	pthread_join(listen, NULL);

	// close connections
	close(sockfd);
}