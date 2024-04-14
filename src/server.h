#ifndef SERVER_H
#define SERVER_H

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
#define THRESHOLD 100 // Threshold in milliseconds

/**
 * @brief Represents the server side of the network compression detection application
 *
 */
typedef struct
{
    /** Socket object */
    int sockfd;

    /** IPv4 socket address */
    struct sockaddr_in servaddr;

    /** Message len */
    socklen_t clilen;

    /** Buffere told hold message */
    char buffer[PACKET_SIZE];

    /** Time variables */
    double start_time, end_time;
} Server;

/**
 * @brief Initialize the server
 *
 * @return Server pointer
 */
void start_server(Server *server)
{
    if (server == NULL)
    {
        perror("Error initializing Server: Memory allocation failed");
    }

    // Create UDP socket
    server->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server->sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address struct
    memset(&server->servaddr, 0, sizeof(server->servaddr));
    server->servaddr.sin_family = AF_INET;
    server->servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Default interface
    server->servaddr.sin_port = htons(SERVER_PORT);
}

/**
 * @brief Receives two sets of UDP packets from the client.
 *
 * @param server this object pointer
 * @return int 0 if recieved successful, -1 otherwise
 */
int receive_packet(Server *server)
{
    int rc = bind(server->sockfd, (const struct sockaddr *)&server->servaddr,
                  sizeof(server->servaddr));

    if (rc == -1)
    {
        perror("Failed to bind");
        close(server->sockfd);
        exit(EXIT_FAILURE);
    }
    server->clilen = 0;
    int n = recvfrom(server->sockfd, server->buffer, PACKET_SIZE, 0,
                     (struct sockaddr *)&server->servaddr, &server->clilen);

    server->buffer[n] = '\n';
    printf("%s", server->buffer);
    return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief Clean up the Server object. Destructor method for cleaning up resources used by the server.
 *
 */
void close(Server *server)
{
    close(server->sockfd);
    free(server);
}

#endif