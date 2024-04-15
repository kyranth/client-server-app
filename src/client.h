#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_PORT 12345
#define PACKET_SIZE 1024

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
        perror("Error initializing Client");
    }

    // Create UDP socket as IPv4
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Set the server address to all 0s
    memset(&client->serv_addr, 0, sizeof(client->serv_addr));

    // IPv4 protocol
    client->serv_addr.sin_family = AF_INET;

    // Setting server port
    client->serv_addr.sin_port = htons(SERVER_PORT);

    // Send to deafault interface "0.0.0.0"
    client->serv_addr.sin_addr.s_addr = INADDR_ANY;
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
        perror("Error: failed to send");
        return -1;
    }

    return 0;
}

#endif