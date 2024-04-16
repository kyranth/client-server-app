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
    struct sockaddr_in servaddr;

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

    // Set the server address to all 0s
    memset(&client->servaddr, 0, sizeof(client->servaddr));

    // IPv4 protocol
    client->servaddr.sin_family = AF_INET;

    // Setting server port
    client->servaddr.sin_port = htons(SERVER_PORT);

    // Set the IP address
    client->servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create UDP socket as IPv4
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    if (connect(client->sockfd, (struct sockaddr *)&client->servaddr, sizeof(client->servaddr)) < 0)
    {
        printf("\nError: Failed to connect\n");
        exit(EXIT_FAILURE);
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
    // Send meessage
    client->len = sendto(client->sockfd, msg, PACKET_SIZE, 0, (const struct sockaddr *)&client->servaddr, sizeof(client->servaddr));
    if (client->len < 0)
    {
        perror("Error: failed to send");
        return -1;
    }

    // waiting for response
    recvfrom(client->sockfd, client->buffer, sizeof(client->buffer), 0, (struct sockaddr *)NULL, NULL);

    puts(client->buffer);
    return 0;
}

int main()
{
    Client *client = (Client *)malloc(sizeof(Client));
    printf("Created Client");

    start_client(client);

    /** Send the first packet train */
    int send = send_packet(client, "Hello there!");
    if (send < 0)
    {
        printf("Error: Counldn't send packet");
    }

    sleep(1);
    close(client->sockfd);
    free(client);

    return 0;
}