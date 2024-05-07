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

// UDP packet structure
typedef struct
{
    uint16_t packet_id;
    char *payload;
} UDP_Packet;

void p_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int init_tcp()
{
    int sockfd;
    // Creating TCP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("ERROR: TCP Socket creation failed\n");
        return -1;
    }
    return sockfd;
}

int init_udp()
{
    int sockfd;
    // Creating UDP socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        p_error("ERROR: UDP Socket creation failed\n");
        return -1;
    }
    return sockfd;
}

Config *createConfig()
{
    Config *config = malloc(sizeof(Config));
    if (config == NULL)
    {
        p_error("ERROR: Failed to allocate memory for Config");
    }
    // Allocate memory for server_ip_address
    config->server_ip_address = malloc(16 * sizeof(char));

    if (config->server_ip_address == NULL)
    {
        p_error("ERROR: Failed to allocate memory for server_ip_address");
    }
    return config;
}

/**
 * @brief Takes connfd and receives config file over socket connection.
 *
 * @param connfd socket file descriptor
 * @return 0 if successful or -1 otherwise
 */
int getConfigFile(int connfd, const char *config_file)
{
    FILE *fp;
    fp = fopen(config_file, "w");

    // Hold incoming data
    ssize_t buffer_size = 1024;
    char buffer[1024];
    ssize_t bytes_received;
    while ((bytes_received = recv(connfd, buffer, buffer_size, 0)) > 0)
    {
        if (bytes_received == 0)
        {
            printf("Client closed connection unexpectedly\n");
            return -1;
        }
        else if (bytes_received == -1)
        {
            printf("ERROR: Couldn't receive file\n");
            return -1;
        }
        fprintf(fp, "%s", buffer);
        memset(buffer, 0, buffer_size);
    }
    close(connfd);

    // Close file
    fclose(fp);

    // Close connection file descriptor
    printf("Successfully received config file. TCP Connection Closed!\n");
    return 0;
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
        p_error("Could not parse config file\n");
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
 * @brief Main function responsible for the client application.
 *
 * @param argc
 * @param argv
 */
int main(int argc, char *argv[])
{
    /** Check for command line arguments */
    if (argc < 1)
    {
        printf("Error: Expected (1) argument Server Port\n");
        exit(0);
    }
    int server_port = atoi(argv[1]); // Get server port from command line

    /** --------- Create socket connection --------- */
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create TCP socket
    sockfd = init_tcp();

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    // Bind TCP socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("ERROR: Bind failed\n");
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0)
    {
        p_error("ERROR: Listen failed\n");
    }

    // Accept incoming connection
    socklen_t len = sizeof(cliaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
    {
        p_error("ERROR: Accept failed\n");
    }

    /** --------- End of TCP Socket Setup --------- */
    /** --------- Pre-Probing Phase: Receive and process config file --------- */
    // Config File
    char *config_file = {"recv_config.json"};
    int process = getConfigFile(connfd, config_file); /* Close TCP connection if successful */
    if (process < 0)
    {
        p_error("ERROR: Didn't receive config file\n");
    }
    close(sockfd);

    // Create and setup config variables
    Config *config = createConfig();
    setConfig(config_file, config);

    // Initiate UDP Socket Connection
    sockfd = init_udp();
    cliaddr.sin_port = htons(config->udp_source_port);
    servaddr.sin_port = htons(config->udp_destination_port);

    // Bind for UDP socket connection
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("ERROR: Bind Failure\n");
    }

    /** --------- End of Pre-Probing Phase --------- */
    /** --------- Probing Phase: Receive Low Entropy Packet Train --------- */

    // Set timeout in seconds
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        p_error("setsockopt (receive timeout) failed");
    }

    struct timeval low, high;
    struct timeval first, last;

    size_t payload_size = (size_t)(config->udp_payload_size - sizeof(uint16_t));
    int num_packets = config->num_udp_packets;

    // UDP packet structure
    typedef struct
    {
        unsigned short packet_id;
        char payload[payload_size];
    } UDP_Packet;

    UDP_Packet low_packet;

    printf("Listening for low entropy packets...\n");
    gettimeofday(&first, NULL); // Record first packet arrival time
    // [1] Receive low entropy data
    ssize_t bytes_received;
    for (int i = 0; i < num_packets; ++i)
    {
        if ((bytes_received = recvfrom(sockfd, &low_packet, sizeof(low_packet.payload), 0, (struct sockaddr *)&cliaddr, &len)) < 0)
        {
            printf("ERROR: Couldn't receive packet!\n");
            break;
        }
        else if (bytes_received == 0)
        {
            printf("Client closed connection\n");
            break;
        }
        printf("Low Entropy : %d packets received!\n", low_packet.packet_id);
        memset(&low_packet, 0, sizeof(low_packet)); // reset packets
    }

    gettimeofday(&last, NULL); // Record the last packet arrival time
    low.tv_sec = last.tv_sec - first.tv_sec;

    // [2] Receive high entropy data
    UDP_Packet packet;
    printf("Listening for high entropy packets...\n");
    gettimeofday(&first, NULL); // Record first packet arrival time
    for (int i = 0; i < num_packets; ++i)
    {
        if ((bytes_received = recvfrom(sockfd, &packet, sizeof(packet.payload), 0, (struct sockaddr *)&cliaddr, &len)) < 0)
        {
            printf("ERROR: Couldn't receive packet!\n");
            break;
        }
        else if (bytes_received == 0)
        {
            printf("Client closed connection\n");
            break;
        }
        printf("High Entropy : %d packets received!\n", packet.packet_id);
        memset(&packet, 0, sizeof(packet)); // reset packets
    }
    gettimeofday(&last, NULL); // Record the last packet arrival time
    high.tv_sec = last.tv_sec - first.tv_sec;

    close(sockfd);

    /** --------- End of Probing Phase --------- */
    /** --------- Post-Probing Phase: Check for compression and Send findings --------- */

    sockfd = init_tcp();
    cliaddr.sin_port = htons(config->tcp_post_probing_port);
    servaddr.sin_port = htons(config->tcp_post_probing_port);

    printf("Post Probing Phase: Initiating TCP Connection (%s/%d)...\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    // Bind TCP socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("ERROR: Bind failed\n");
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0)
    {
        p_error("ERROR: Listen failed\n");
    }

    // Accept incoming connection
    len = sizeof(servaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&servaddr, &len)) < 0)
    {
        p_error("ERROR: Accept failed\n");
    }

    printf("Delta Low Entropy: %ld\n", low.tv_sec);
    printf("Delta High Entropy: %ld\n", high.tv_sec);
    printf("D_High - D_Low: %ld\n", high.tv_sec - low.tv_sec);

    if ((high.tv_sec - low.tv_sec) > config->inter_measurement_time)
    {
        char compression[] = {"Compression detected!"};
        send(connfd, compression, strlen(compression), 0);
    }
    else
    {
        char no_compression[] = {"No Compression detected!"};
        send(connfd, no_compression, strlen(no_compression), 0);
    }
    printf("Result Sent!\n");

    /** Close TCP connection */
    close(sockfd);
    close(connfd);
    free(config);

    printf("Server Concluded!\n");
    return 0;
}
