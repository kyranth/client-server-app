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
#include <time.h>

#define BUFFER_SIZE 1024
#define THRESHOLD 100

// Structure definition
typedef struct
{
    char *server_ip_address; // Assuming IPv4 address, so 15 characters + null terminator
    int tcp_pre_probing_port;
    int udp_source_port;
    int udp_destination_port;
    int tcp_head_syn_port;
    int tcp_tail_syn_port;
    int udp_payload_size;
    int inter_measurement_time;
    int udp_packet_train_size;
    int udp_packet_ttl;
} Config;

Config *createConfig()
{
    Config *config = malloc(sizeof(Config));
    if (config == NULL)
    {
        perror("Failed to allocate memory for Config");
        exit(1);
    }

    // Allocate memory for server_ip_address
    config->server_ip_address = malloc(16 * sizeof(char)); // Assuming IPv4 address, so 15 characters + null terminator
    // config->tcp_pre_probing_port = malloc(8 * sizeof(char));

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
 * @brief Takes a filename and returns the cJSON obejct
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
        // /** Temp hardcoding them */
        // strcpy(config->server_ip_address, "127.0.0.1");
        // config->tcp_pre_probing_port = 7777;

        strcpy(config->server_ip_address,
               cJSON_GetObjectItemCaseSensitive(json, "server_ip_address")->valuestring);
        config->tcp_pre_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valueint;

        // strcpy(config.tcp_pre_probing_port, cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valuestring);
        config->udp_source_port = cJSON_GetObjectItemCaseSensitive(json, "udp_source_port")->valueint;
        config->udp_destination_port = cJSON_GetObjectItemCaseSensitive(json, "udp_destination_port")->valueint;
        config->tcp_head_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_head_syn_port")->valueint;
        config->tcp_tail_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_tail_syn_port")->valueint;
        config->udp_payload_size = cJSON_GetObjectItemCaseSensitive(json, "udp_payload_size")->valueint;
        config->inter_measurement_time = cJSON_GetObjectItemCaseSensitive(json, "inter_measurement_time")->valueint;
        config->udp_packet_train_size = cJSON_GetObjectItemCaseSensitive(json, "udp_packet_train_size")->valueint;
        config->udp_packet_ttl = cJSON_GetObjectItemCaseSensitive(json, "udp_packet_ttl")->valueint;
    }

    // delete the JSON object
    cJSON_Delete(json);
}

int init_tcp()
{
    int sockfd;
    // Creating TCP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("ERROR: TCP Socket creation failed\n");
    }
    return sockfd;
}

void init_udp()
{
    int sockfd;
    // Creating UDP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
    {
        p_error("ERROR: UDP Socket creation failed\n");
    }
}

int send_file(int sockfd)
{
    /** Create file */
    FILE *config_file;
    char buffer[BUFFER_SIZE] = {0};

    /** Open and read file */
    config_file = fopen("config.json", "r");
    if (config_file == NULL)
    {
        p_error("Error opening config file\n");
    }

    /** Read filesize */
    fseek(config_file, 0, SEEK_END);
    uint64_t size = ftell(config_file);
    printf("File size: %ld\n", size);
    send(sockfd, &size, sizeof(size), 0);

    // sleep(1);

    while (fgets(buffer, BUFFER_SIZE, config_file) != NULL)
    {
        if (send(sockfd, buffer, BUFFER_SIZE, 0) == -1)
        {
            p_error("Error sending file\n");
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    fclose(config_file);
    printf("File closed. Waiting for confirmation...\n");

    // Receive confirmation message
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0)
    {
        p_error("Receive failed");
        return -1;
    }
    else if (bytes_received == 0)
    {
        printf("Server closed connection\n");
        close(sockfd);
    }
    else
    {
        buffer[bytes_received] = '\0';
        printf("Server: %s\n", buffer);
    }

    return 0;
}

// Function to fill UDP header
// void fill_udp_header(struct udphdr *udp_header, uint16_t source_port, uint16_t dest_port, uint16_t payload_size)
// {
//     udp_header->source = htons(source_port);
//     udp_header->dest = htons(dest_port);
//     udp_header->len = htons(sizeof(struct udphdr) + payload_size);
//     udp_header->check = 0; // Checksum calculation is optional
// }

// Function to fill IP header
// void fill_ip_header(struct iphdr *ip_header, char *source_ip, char *dest_ip, uint16_t payload_size)
// {
//     ip_header->ihl = 5;
//     ip_header->version = 4;
//     ip_header->tos = 0;
//     ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size);
//     ip_header->id = htons(0);           // Can be any value
//     ip_header->frag_off = htons(IP_DF); // Don't fragment flag
//     ip_header->ttl = 64;                // Time to live
//     ip_header->protocol = IPPROTO_UDP;
//     ip_header->check = 0; // Checksum calculation is optional
//     ip_header->saddr = inet_addr(source_ip);
//     ip_header->daddr = inet_addr(dest_ip);
// }

// Function to construct and send UDP packet
// void send_udp_packet(int sockfd, struct sockaddr_in servaddr, char *source_ip, char *dest_ip, uint16_t source_port, uint16_t dest_port, uint16_t packet_id, char *payload, uint16_t payload_size)
// {
//     char buffer[PACKET_SIZE];
//     struct iphdr *ip_header = (struct iphdr *)buffer;
//     struct udphdr *udp_header = (struct udphdr *)(buffer + sizeof(struct iphdr));
//     char *packet_payload = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

//     // Fill IP header
//     fill_ip_header(ip_header, source_ip, dest_ip, payload_size);

//     // Fill UDP header
//     fill_udp_header(udp_header, source_port, dest_port, payload_size);

//     // Fill packet payload
//     *(uint16_t *)packet_payload = htons(packet_id);
//     memcpy(packet_payload + sizeof(uint16_t), payload, payload_size - sizeof(uint16_t));

//     // Calculate IP checksum
//     ip_header->check = checksum((unsigned short *)buffer, sizeof(struct iphdr));

//     // Send packet
//     sendto(sockfd, buffer, sizeof(struct iphdr) + sizeof(struct udphdr) + payload_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
// }

// void send_packet_train(int sockfd, struct sockaddr_in servaddr, int entropy)
// {
//     char packet[PACKET_SIZE];
//     // struct timeval start_time, end_time;
//     // gettimeofday(&start_time, NULL);
//     for (int i = 0; i < 2; ++i)
//     {
//         memset(packet, 0, PACKET_SIZE);
//         *(uint16_t *)packet = htons(i); // Packet ID
//         sendto(sockfd, packet, PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
//     }
//     // gettimeofday(&end_time, NULL);
//     if (entropy)
//         sleep(INTER_MEASUREMENT_TIME); // Wait before sending high entropy data
// }

int main(int argc, char *argv[])
{
    /** Check for commandline arguments */
    if (argc < 1)
    {
        printf("Error: Expected (1) arguement config.json file\n");
        exit(0);
    }
    char *config_file = argv[1];

    /** Create sokcet file descriptors */
    struct sockaddr_in servaddr;

    // double low_entropy_time, high_entropy_time;

    /** Initialize a config structure to NULL values */
    Config *config = createConfig();

    /** Read the config file for server IP and fill it with JSON data */
    setConfig(config_file, config);
    printf("Server IP: %s\n", config->server_ip_address);
    printf("Pre prob port: %d\n", config->tcp_pre_probing_port);

    /** Init TCP Connection */
    int sockfd = init_tcp();

    /** Set server address */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->tcp_pre_probing_port);

    /** Connect to server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Connection Failed");
    }

    /** Pre-Probing Phase: Send config file */
    send_file(sockfd);

    /** Probing Phase: Packet training */

    /** Generate low entropy data (all 0s) */
    // char low_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    // memset(low_entropy_data, 0, sizeof(low_entropy_data));

    /** Generate high entropy data (random numbers) */
    // srand(time(NULL));
    // char high_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    // for (int i = 0; i < sizeof(high_entropy_data); ++i)
    // {
    //     high_entropy_data[i] = rand() % 256;
    // }

    /** Construct and send packet train */
    // for (int i = 0; i < 10; ++i)
    // {
    // Send low entropy data packet
    //     send_udp_packet(sockfd, servaddr, "192.168.1.100", SERVER_IP, SOURCE_PORT, DEST_PORT, i, low_entropy_data, sizeof(low_entropy_data));

    // Wait before sending high entropy data
    //     sleep(INTER_MEASUREMENT_TIME);

    // Send high entropy data packet
    //     send_udp_packet(sockfd, servaddr, "192.168.1.100", SERVER_IP, SOURCE_PORT, DEST_PORT, i + 100, high_entropy_data, sizeof(high_entropy_data));
    // }

    /** Close TCP connection */
    // close(sockfd);

    // Check for compression
    // if ((high_entropy_time - low_entropy_time) > THRESHOLD)
    // {
    //     printf("Compression detected!\n");
    // }
    // else
    // {
    //     printf("No compression was detected.\n");
    // }
    free(config);
    return 0;
}
