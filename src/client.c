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

    ssize_t buffer_size = 1024;
    ssize_t bytes = 0;
    char buffer[1024] = {0};
    while (fgets(buffer, buffer_size, config_file) != NULL)
    {
        bytes = send(sockfd, buffer, buffer_size, 0);
        if (bytes < 0)
        {
            printf("ERROR: Couldn't send file\n");
            return -1;
        }
        memset(buffer, 0, buffer_size); // reset buffer
    }
    // Close file
    fclose(config_file);
    return 0;
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
        p_error("ERROR: Don't Fragment failed\n");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("ERROR: TCP Connection Failed\n");
    }

    /** --------- End of Socket setup --------- */
    /** --------- Pre-Probing Phase: Send config file --------- */

    printf("Pre-Probing Phase: Sending Config file to (%s/%d)...\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    int send = send_ConfigFile(sockfd);
    if (send < 0)
    {
        p_error("ERROR: Couldn't send config file\n");
    }

    // Release and reset TCP Connection
    close(sockfd);
    memset(&cliaddr, 0, sizeof(cliaddr));
    printf("Config file sent and TCP Connection released!\n");

    /** --------- End of Pre-Probing Phase --------- */
    /** --------- Probing Phase: Sending low and high entropy train --------- */

    // [1] Low Entropy - Initiate UDP Connection
    sockfd = init_udp();
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(config->udp_source_port);

    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->udp_destination_port);
    printf("Probing Phase: Initiating UDP Connection with (%s/%d)...\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    // UDP Bind
    if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        p_error("ERROR: Bind failed\n");
    }

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
    printf("Sending Low Entropy Packet Train...\n");
    for (int i = 0; i < num_packets; ++i)
    {
        // Prepare packet payload with packet ID
        low_packet.packet_id = (unsigned short)i;

        // Send
        sendto(sockfd, &low_packet, sizeof(low_packet.payload), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // Wait to prevent sending packets too fast
        usleep(200);
    }
    printf("Low entropy packet train sent!\n");

    // Wait before sending high entropy data
    printf("Waiting for inter-measurement time...\n");
    sleep(config->inter_measurement_time);

    // [2] High Entropy - Initiate UDP Connection
    // Get generated high entropy data (random numbers) from file
    char rand_data[payload_size - 2];
    FILE *random = fopen("random_file", "r");

    // Read and close random file
    fread(rand_data, 1, sizeof(rand_data), random);
    fclose(random);

    UDP_Packet high_packet;

    // Copy high entropy data to payload
    memcpy(high_packet.payload, rand_data, payload_size);

    // Send high entropy data packet
    printf("Sending High Entropy Packet Train...\n");
    for (int i = 0; i < num_packets; ++i)
    {
        // Prepare packet payload with packet ID
        high_packet.packet_id = (unsigned short)i;

        // Send Packet
        sendto(sockfd, &high_packet, sizeof(high_packet.payload), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // Wait to prevent sending packets too fast
        usleep(200);
    }
    printf("High entropy packet train sent. UDP Socket Connection Closed!\n");
    close(sockfd);

    /** --------- End of Probing Phase --------- */
    /** --------- Post Probing Phase: Receive Compression Result --------- */

    // Init TCP Connection for receiving result
    sockfd = init_tcp();

    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliaddr.sin_port = htons(config->tcp_post_probing_port);
    servaddr.sin_addr.s_addr = inet_addr(config->server_ip_address);
    servaddr.sin_port = htons(config->tcp_post_probing_port);
    printf("Post Probing Phase: Starting TCP Connection with (%s/%d)...\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

    if (connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
    {
        p_error("TCP Connection Failed\n");
    }

    // Variables for receive
    char buffer[25];
    ssize_t bytes_received;

    printf("Waiting for result from (%s/%d)...\n", inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer), 0)) > 0)
    {
        printf("%s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    close(sockfd);
    free(config);

    printf("Client Concluded!\n");
    return 0;
}
