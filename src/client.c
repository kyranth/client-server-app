#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/cJSON.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
// #include <time.h>

#define BUFFER_SIZE 1024
#define PACKET_SIZE 1000
#define INTER_MEASUREMENT_TIME 15

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

typedef struct
{
    uint16_t packet_id;
    char payload[PACKET_SIZE - sizeof(uint16_t)];
} UDP_Packet;

Config *createConfig()
{
    Config *config = malloc(sizeof(Config));
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
 * Creates a TCP file descriptor and returns for use.
 *
 * @return sockfd file descriptor
 */
int init_tcp()
{
    int sockfd;
    // Creating TCP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: TCP Socket creation failed\n");
        return -1;
    }
    return sockfd;
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

/**
 * @brief Takes sockfd and sends config file over connection
 *
 * @param sockfd socket file descriptor or -1 if unsuccessful
 */
int send_ConfigFile(int sockfd)
{
    /** Create file */
    FILE *config_file;

    /** Open and read file */
    config_file = fopen("config.json", "r");
    if (config_file == NULL)
    {
        printf("ERROR: Opening config file\n");
        return -1;
    }

    ssize_t bytes = 0;
    char buffer[BUFFER_SIZE] = {0};
    while (fgets(buffer, BUFFER_SIZE, config_file) != NULL)
    {
        bytes = send(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes < 0)
        {
            printf("ERROR: Couldn't send file\n");
            return -1;
        }
        memset(buffer, 0, BUFFER_SIZE); // reset buffer
    }
    // Close file
    fclose(config_file);
    return 0;
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

    /** --------- Init TCP Socket Connection --------- */
    int sockfd = init_tcp();
    struct sockaddr_in servaddr, cliaddr;

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->tcp_pre_probing_port);

    // Enable Don't Fragment flag
    int enable = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &enable, sizeof(enable)) < 0)
    {
        p_error("setsockopt failed\n");
    }
    printf("IP/Port: %s/%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("TCP Connection Failed\n");
    }
    /** --------- End of Socket setup --------- */
    /** --------- Pre-Probing Phase: Send config file --------- */
    printf("Pre-Probing Phase: Sending Config file...\n");
    int send = send_ConfigFile(sockfd);
    if (send < 0)
    {
        p_error("Couldn't send config file\n");
    }

    // Release and reset TCP Connection
    close(sockfd);
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    printf("Config file sent and TCP Connection released!\n");

    /** --------- End of Pre-Probing Phase --------- */
    /** --------- Probing Phase: Sending low and high entropy train --------- */

    // Initiate UDP Connection
    printf("Probing Phase: Initiating UDP Connection...\n");
    sockfd = init_udp();
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(config->udp_source_port);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->udp_destination_port);
    printf("IP/Port: %s/%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        p_error("ERROR: Bind failed\n");
    }

    // Send low entropy data packet
    printf("Sending Low Entropy Packet Train...\n");
    int num_packets = 100;
    UDP_Packet packet[num_packets];
    for (int i = 0; i < num_packets; i++)
    {
        // Prepare packet payload with packet ID
        packet[i].packet_id = htons(i);

        // Generate low entropy payload data (all 0s)
        memset(packet[i].payload, 0, sizeof(packet[i].payload));

        // Send
        sendto(sockfd, &packet[i], PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        // printf("Sent packet with ID: %d\n", ntohs(packet[i].packet_id));

        // Wait 50 Millisec to prevent sending packets too fast
        usleep(50000);
    }
    printf("Low Entropy Packet Train Sent!\n");

    // Wait before sending high entropy data
    printf("Waiting for inter-measurement time...\n");
    sleep(15);

    memset(&packet, 0, sizeof(packet)); /* Reset packet data from previous packet train */

    // Get generated high entropy data (random numbers) from file
    char rand_data[PACKET_SIZE];
    FILE *random = fopen("random_file", "r");

    // Read and close random file
    fread(rand_data, 1, sizeof(rand_data), random);
    fclose(random);

    // Send high entropy data packet
    printf("Sending High Entropy Packet Train...\n");
    for (int i = 0; i < num_packets; i++)
    {
        // Prepare packet payload with packet ID
        packet[i].packet_id = htons(i);

        // Copy high entropy data to payload
        memcpy(packet[i].payload, rand_data, sizeof(packet[i].payload));

        // Send Packet
        sendto(sockfd, &packet[i], PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        // printf("Sent packet with ID: %d\n", ntohs(packet[i].packet_id));

        // Wait 50 Millisec to prevent sending packets too fast
        usleep(50000);
    }
    printf("High Entropy Packet Train sent!\n");

    printf("Closing UDP Socket Connection...\n");
    close(sockfd);
    memset(&cliaddr, 0, sizeof(cliaddr));

    /** --------- End of Probing Phase --------- */
    /** --------- Post Probing Phase: Receive Compression Result --------- */

    // Init TCP Connection for receiving result
    sockfd = init_tcp();
    int connfd;
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliaddr.sin_port = htons(config->tcp_post_probing_port);

    // Bind TCP socket
    if (bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        p_error("ERROR: Bind failed\n");
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0)
    {
        p_error("ERROR: Listen failed\n");
    }

    // Accept incoming connection
    socklen_t len = sizeof(servaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&servaddr, &len)) < 0)
    {
        p_error("ERROR: Accept failed\n");
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(connfd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        printf("%s\n", buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(sockfd);
    free(config);

    printf("Connection closed\n");
    return 0;
}
