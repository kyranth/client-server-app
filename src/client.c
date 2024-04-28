#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>  // For IP_HDRINCL
#include <netinet/udp.h> // For UDP header

/** Constant members */
#define SERVER_IP "172.16.4.4" // Update for server address
#define CLIENT_IP "172.16.4.2" // Update for client address
#define SERVER_PORT 8765
#define CLIENT_PORT 9876
#define PACKET_SIZE 1024

/**
 * @brief Represents the client side of the network compression detection application
 *
 */
typedef struct
{
    /** Socket object */
    int sockfd;

    /** Variable hold message */
    char msg_buffer[PACKET_SIZE];

    /** IPv4 socker server address */
    struct sockaddr_in servaddr, cliaddr;
} Client;

/**
 * @brief Sets up the client socket connection.
 * Assigns server address to 0s, then sets ip address family to IPv4.
 * Follows the similar pattern for client socket connection.
 *
 * @param client struct object
 */
void start_client(Client *client)
{
    if (client == NULL)
    {
        perror("ERROR: Client was not initiated\n");
    }

    /** Configure Client struct sockaddr_in */
    memset(&client->servaddr, 0, sizeof(client->servaddr)); // Set the server address to all 0s
    client->servaddr.sin_family = AF_INET;                  // IPv4 protocol
    client->servaddr.sin_port = SERVER_PORT;                // Setting server port
    client->servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // memset(&client->cliaddr, 0, sizeof(client->cliaddr));
    // client->cliaddr.sin_family = AF_INET;
    // client->cliaddr.sin_port = htons(CLIENT_PORT);
    // client->cliaddr.sin_addr.s_addr = inet_addr(CLIENT_IP);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR: Creating socket\n");
        exit(EXIT_FAILURE);
    }

    int DF = IP_PMTUDISC_DO; // Don't fragment
    setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &DF, sizeof(DF));

    // Connect to server
    if (connect(client->sockfd, (struct sockaddr *)&client->servaddr, sizeof(client->servaddr)) < 0)
    {
        perror("ERROR: connection failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Connection successfull!\n");
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
    sendto(client->sockfd, msg, PACKET_SIZE, 0, (const struct sockaddr *)&client->servaddr, sizeof(client->servaddr));

    // waiting for response
    recvfrom(client->sockfd, client->msg_buffer, sizeof(client->msg_buffer), 0, (struct sockaddr *)NULL, NULL);

    puts(client->msg_buffer);
    return 0;
}

/**
 * @brief Takes a filename pointer and socket object
 * and sends the file over the socket connection.
 *
 * @param *file file pointer
 * @param sockfd socket object
 */
void send_file(const char *file, int sockfd)
{
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        perror("ERROR: File doesn't exist\n");
        exit(EXIT_FAILURE);
    }

    char data[PACKET_SIZE] = {0};

    while (fgets(data, PACKET_SIZE, fp) != NULL)
    {
        if (send(sockfd, data, sizeof(data), 0) == -1)
        {
            perror("ERROR: Couldn't send file");
            exit(EXIT_FAILURE);
        }
        memset(data, 0, PACKET_SIZE);
    }

    fclose(fp);
}

int main()
{
    /** Instantiate the client */
    Client *client = (Client *)malloc(sizeof(Client));
    start_client(client);

    /** Sending the file */
    // char *filename = "../config.json";
    // send_file(filename, client->sockfd);

    // Send packets with delay
    for (int i = 0; i < 2; ++i)
    {
        send_packet(client, "Hello, there");
        sleep(3); // Sleep for specified delay in microseconds
        printf("Packet sent!\n");
    }

    /** Close the socket connection */
    close(client->sockfd);

    /** Release resources for */
    free(client);

    return 0;
}