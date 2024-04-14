#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_PORT 12345
#define PACKET_SIZE 1024
#define NUM_PACKETS 100

/**
 * @brief Represents the client side of the network compression detection application
 *
 */
typedef struct
{
    /** Socket object */
    int sockfd;

    /** IPv4 socker server address */
    struct sockaddr_in serv_addr;

    /** Variable hold message */
    char buffer[PACKET_SIZE];

    /** Send confirmation */
    int len;

} Client;

void start_client(Client *client)
{
    if (client == NULL)
    {
        error("Error initializing Server: Memory allocation failed");
    }

    // Create UDP socket as IPv4
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&client->serv_addr, 0, sizeof(client->serv_addr));
    client->serv_addr.sin_family = AF_INET;
    client->serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &client->serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
    }
}

/**
 * @brief Sends a packet train, with the message
 *
 * @param client this client object
 * @param msg message
 * @return int 0 if recieved successful, -1 otherwise
 */
int send_packet(Client *client, char *msg)
{
    client->len = sendto(client->sockfd, msg, PACKET_SIZE, 0,
                         (const struct sockaddr *)&client->serv_addr, sizeof(client->serv_addr));

    if (client->len < 0)
    {
        error("Error sending second packet train");
        return -1;
    }

    return 0;
}

/**
 * @brief Generates the given error message
 *
 * @param msg error message
 */
void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief Closes the client object
 *
 * @param client struct
 */
void close(Client *client)
{
    close(client->sockfd);
    free(client);
}

#endif