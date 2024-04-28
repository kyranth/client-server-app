#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <time.h>

#define SERVER_IP "172.17.0.2"
#define SERVER_PORT 8080
#define SIZE 1024
#define PACKET_SIZE 1024
#define INTER_MEASUREMENT_TIME 1
#define THRESHOLD 100

void p_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_config(int sockfd)
{
    FILE *config_file;
    char buffer[SIZE];
    config_file = fopen("config.json", "r");
    if (config_file == NULL)
    {
        p_error("Error opening config file.");
    }
    while (fgets(buffer, SIZE, config_file) != NULL)
    {
        send(sockfd, buffer, strlen(buffer), 0);
    }
    fclose(config_file);
}

// Function to fill UDP header
void fill_udp_header(struct udphdr *udp_header, uint16_t source_port, uint16_t dest_port, uint16_t payload_size)
{
    udp_header->source = htons(source_port);
    udp_header->dest = htons(dest_port);
    udp_header->len = htons(sizeof(struct udphdr) + payload_size);
    udp_header->check = 0; // Checksum calculation is optional
}

// Function to fill IP header
void fill_ip_header(struct iphdr *ip_header, char *source_ip, char *dest_ip, uint16_t payload_size)
{
    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->tos = 0;
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
    ip_header->id = htons(0);           // Can be any value
    ip_header->frag_off = htons(IP_DF); // Don't fragment flag
    ip_header->ttl = 64;                // Time to live
    ip_header->protocol = IPPROTO_UDP;
    ip_header->check = 0; // Checksum calculation is optional
    ip_header->saddr = inet_addr(source_ip);
    ip_header->daddr = inet_addr(dest_ip);
}

// Function to construct and send UDP packet
void send_udp_packet(int sockfd, struct sockaddr_in servaddr, char *source_ip, char *dest_ip, uint16_t source_port, uint16_t dest_port, uint16_t packet_id, char *payload, uint16_t payload_size)
{
    char buffer[PACKET_SIZE];
    struct iphdr *ip_header = (struct iphdr *)buffer;
    struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct iphdr));
    char *packet_payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

    // Fill IP header
    fill_ip_header(ip_header, source_ip, dest_ip, payload_size);

    // Fill UDP header
    fill_udp_header(udp_header, source_port, dest_port, payload_size);

    // Fill packet payload
    *(uint16_t *)packet_payload = htons(packet_id);
    memcpy(packet_payload + sizeof(uint16_t), payload, payload_size - sizeof(uint16_t));

    // Calculate IP checksum
    ip_header->check = checksum((unsigned short *)buffer, sizeof(struct iphdr));

    // Send packet
    sendto(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
}

void send_packet_train(int sockfd, struct sockaddr_in servaddr, int n, int entropy)
{
    char packet[PACKET_SIZE];
    // struct timeval start_time, end_time;
    // gettimeofday(&start_time, NULL);
    for (int i = 0; i < 2; ++i)
    {
        memset(packet, 0, PACKET_SIZE);
        *(uint16_t *)packet = htons(i); // Packet ID
        sendto(sockfd, packet, PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
    // gettimeofday(&end_time, NULL);
    if (entropy)
        sleep(INTER_MEASUREMENT_TIME); // Wait before sending high entropy data
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    // struct timeval start_time_low, end_time_low, start_time_high, end_time_high;
    // double low_entropy_time, high_entropy_time;

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("Socket creation failed");
    }

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Connection Failed");
    }

    // Pre-Probing Phase: Send config file
    // send_config(sockfd);

    // Generate low entropy data (all 0s)
    char low_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    memset(low_entropy_data, 0, sizeof(low_entropy_data));

    // Generate high entropy data (random numbers)
    srand(time(NULL));
    char high_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    for (int i = 0; i < sizeof(high_entropy_data); ++i)
    {
        high_entropy_data[i] = rand() % 256;
    }

    // Construct and send packet train
    for (int i = 0; i < 10; ++i)
    {
        // Send low entropy data packet
        send_udp_packet(sockfd, servaddr, "192.168.1.100", SERVER_IP, SOURCE_PORT, DEST_PORT, i, low_entropy_data, sizeof(low_entropy_data));

        // Wait before sending high entropy data
        sleep(INTER_MEASUREMENT_TIME);

        // Send high entropy data packet
        send_udp_packet(sockfd, servaddr, "192.168.1.100", SERVER_IP, SOURCE_PORT, DEST_PORT, i + 100, high_entropy_data, sizeof(high_entropy_data));
    }

    // Close TCP connection
    close(sockfd);

    // Calculate time differences
    // low_entropy_time = (end_time_low.tv_sec - start_time_low.tv_sec) * 1000.0 + (end_time_low.tv_usec - start_time_low.tv_usec) / 1000.0;
    // high_entropy_time = (end_time_high.tv_sec - start_time_high.tv_sec) * 1000.0 + (end_time_high.tv_usec - start_time_high.tv_usec) / 1000.0;

    // Check for compression
    // if ((high_entropy_time - low_entropy_time) > THRESHOLD)
    // {
    //     printf("Compression detected!\n");
    // }
    // else
    // {
    //     printf("No compression was detected.\n");
    // }

    return 0;
}
