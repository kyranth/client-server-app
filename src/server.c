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

#define SERVER_PORT 7777
#define BUFFER_SIZE 1024
#define PAYLOAD_SIZE 1000

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
    char payload[PAYLOAD_SIZE - sizeof(uint16_t)];
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
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(connfd, buffer, BUFFER_SIZE, 0)) > 0)
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
        memset(buffer, 0, BUFFER_SIZE);
    }
    // Close file
    fclose(fp);

    // Close connection file descriptor
    printf("Successfully received config file. Closing TCP Connection...\n");
    close(connfd);
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
    char buffer[BUFFER_SIZE];
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

int main()
{
    /** --------- Create socket connection --------- */
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create TCP socket
    sockfd = init_tcp();

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

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
    struct timeval low, high;
    struct timeval first, last;

    UDP_Packet packet;
    int n;
    int num_packets = 100;

    // Receive low entropy data
    gettimeofday(&first, NULL); // Record first packet arrival time
    for (int i = 0; i < num_packets - 1; ++i)
    {
        if ((n = recvfrom(sockfd, &packet, PAYLOAD_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len)) < 0)
        {
            printf("ERROR: Received a negative value\n");
            break;
        }
        else if (n == 0)
        {
            printf("Client closed connection\n");
            break;
        }

        printf("Received packet with ID: %d\n", ntohs(packet.packet_id));
        // printf("Payload: ");
        // for (int j = 0; j < sizeof(packet.payload); j++)
        // {
        //     printf("%d", packet.payload[j]);
        // }
        // printf("\n\n");
    }
    gettimeofday(&last, NULL); // Record the last packet arrival time
    printf("Low Entropy received\n");

    low.tv_sec = last.tv_sec - first.tv_sec;
    memset(&packet, 0, sizeof(packet)); // reset packets

    // Receive high entropy data
    gettimeofday(&first, NULL); // Record first packet arrival time
    for (int i = 0; i < num_packets - 1; ++i)
    {
        if ((n = recvfrom(sockfd, &packet, PAYLOAD_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len)) < 0)
        {
            printf("Encountered an error\n");
            break;
        }
        else if (n == 0)
        {
            printf("Client closed connection\n");
            break;
        }

        // printf("Received packet with ID: %d\n", ntohs(packet.packet_id));
        // printf("Payload: ");
        // for (int j = 0; j < sizeof(packet.payload); j++)
        // {
        //     printf("%d", packet.payload[j]);
        // }
        // printf("\n\n");
    }
    gettimeofday(&last, NULL); // Record the last packet arrival time
    printf("High entropy packets received!\n");
    high.tv_sec = last.tv_sec - first.tv_sec;

    printf("Delta Low: %ld\n", low.tv_sec);
    printf("Delta High: %ld\n", high.tv_sec);
    printf("D_High - D_Low: %ld\n", high.tv_sec - low.tv_sec);

    close(sockfd); /* Close UDP Socket Connection */

    /** --------- End of Probing Phase --------- */
    /** --------- Post-Probing Phase: Check for compression and Send findings --------- */

    sockfd = init_tcp();
    cliaddr.sin_port = htons(config->tcp_post_probing_port);
    servaddr.sin_port = htons(config->tcp_post_probing_port);
    printf("IP/Port: %s/%d\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        p_error("TCP Connection Failed\n");
    }

    if ((high.tv_sec - low.tv_sec) > config->inter_measurement_time)
    {
        char compression[] = {"Compression detected!"};
        send(sockfd, compression, strlen(compression), 0);
    }
    else
    {
        char no_compression[] = {"No Compression detected!"};
        send(sockfd, no_compression, strlen(no_compression), 0);
    }
    printf("Result Sent!\n");

    /** Close TCP connection */
    close(sockfd);
    free(config);

    printf("Connection closed\n");
    return 0;
}
