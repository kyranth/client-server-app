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
#include <time.h>

#define BUFFER_SIZE 1024
#define THRESHOLD 100

#define PACKET_SIZE 1000
#define INTER_MEASUREMENT_TIME 15

// Structure definition
typedef struct
{
    char *server_ip_address;
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

void send_packet_train(int sockfd, struct sockaddr_in servaddr, int entropy)
{
    char packet[PACKET_SIZE];
    struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);
    for (int i = 0; i < 2; ++i)
    {
        memset(packet, 0, PACKET_SIZE);
        *(uint16_t *)packet = htons(i); // Packet ID
        sendto(sockfd, packet, PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
    gettimeofday(&end_time, NULL);

    if (entropy)
        sleep(INTER_MEASUREMENT_TIME); // Wait before sending high entropy data
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

    struct sockaddr_in servaddr; // Create socket file descriptors

    /** Initialize a config structure to NULL values */
    Config *config = createConfig();

    /** Read the config file for server IP and fill it with JSON data */
    setConfig(config_file, config);
    printf("Server IP/Port: %s/%d\n", config->server_ip_address, config->tcp_pre_probing_port);

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
        p_error("TCP Connection Failed\n");
    }

    /** Pre-Probing Phase: Send config file */
    int send = send_ConfigFile(sockfd);
    if (send < 0)
    {
        p_error("Couldn't send config file\n");
    }
    close(sockfd); // TCP Connection

    /** Probing Phase: Sending low and high entropy data */
    // [1] Initiate UDP Connection
    // sockfd = init_udp();
    // servaddr.sin_port = htons(config->udp_destination_port);

    // [2] connect to server no need
    // if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    // {
    //     p_error("UDP Connection Failed\n");
    // }

    // [3] Generate low entropy data (all 0s)
    // char low_entropy_data[PACKET_SIZE];
    // memset(low_entropy_data, 0, sizeof(low_entropy_data));

    // [4] Generate high entropy data (random numbers), rand to a file
    // srand(time(NULL));
    // char high_entropy_data[PACKET_SIZE - sizeof(uint16_t)];
    // for (int i = 0; i < sizeof(high_entropy_data); ++i)
    // {
    //     high_entropy_data[i] = rand() % 256;
    // }

    // [5] Construct and send packet train

    // [6] Send low entropy data packet

    // [7] Wait before sending high entropy data
    // sleep(INTER_MEASUREMENT_TIME);

    // [8] Send high entropy data packet

    /** Post Probing Phase: Calculate compression; done on Server */
    // close(sockfd);

    free(config); // Release config
    return 0;
}
