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
    int num_udp_packets;
    int udp_packet_ttl;
} Config;

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
    }
    return sockfd;
}

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
        strcpy(config->server_ip_address,
               cJSON_GetObjectItemCaseSensitive(json, "server_ip_address")->valuestring);
        config->tcp_pre_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valueint;
        config->udp_source_port = cJSON_GetObjectItemCaseSensitive(json, "udp_source_port")->valueint;
        config->udp_destination_port = cJSON_GetObjectItemCaseSensitive(json, "udp_destination_port")->valueint;
        config->tcp_head_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_head_syn_port")->valueint;
        config->tcp_tail_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_tail_syn_port")->valueint;
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
    /** Create socket connection */
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;

    /** Create TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("Socket creation failed");
    }

    /** Set server address */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    /** Bind socket */
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Bind failed");
    }

    /** Listen for connections */
    if (listen(sockfd, 5) < 0)
    {
        p_error("Listen failed");
    }

    /** Accept incoming connection */
    socklen_t len = sizeof(cliaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
    {
        p_error("Accept failed");
    }

    /** Pre-Probing Phase: Receive and process config file */
    char *config_file = {"recv_config.json"};
    int process = getConfigFile(connfd, config_file); // Close TCP connection if successful
    if (process < 0)
    {
        p_error("ERROR: Didn't receive config file\n");
    }

    // Config struct
    Config *config = createConfig();
    setConfig(config_file, config);

    /** Probing Phase: Receive packet trains */
    // struct timeval t1, t2; // time variables
    // char buffer[BUFFER_SIZE];
    // int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
    // buffer[n] = '\0';
    // gettimeofday(&t1, NULL); // Record arrival time of first packet
    // printf("First packet received: %s\n", buffer);

    // Receiving second UDP packet
    // int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
    // buffer[n] = '\0';
    // gettimeofday(&t2, NULL); // Record arrival time of second packet
    // printf("Second packet received: %s\n", buffer);

    /** Post-Probing Phase: Check for compression and Send findings */
    // double low_entropy_time, high_entropy_time;

    // low_entropy_time = (end_time_low.tv_sec - start_time_low.tv_sec) * 1000.0 + (end_time_low.tv_usec - start_time_low.tv_usec) / 1000.0;
    // high_entropy_time = (end_time_high.tv_sec - start_time_high.tv_sec) * 1000.0 + (end_time_high.tv_usec - start_time_high.tv_usec) / 1000.0;

    // char result[25];
    // if ((high_entropy_time - low_entropy_time) > THRESHOLD) // TODO: check for miliseconds or seconds
    // {
    // strcpy(result, "Compression detected!");
    //     send(connfd, result, strlen(result), 0);
    // }
    // else
    // {
    // strcpy(result, "No Compression detected!");
    //     send(connfd, result, strlen(result), 0);
    // }
    //

    /** Close TCP connection */
    close(sockfd);

    return 0;
}
