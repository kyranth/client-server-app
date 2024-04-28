#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/** Constant members */
#define SERVER_IP "172.16.4.4" // Update for server address
#define SERVER_PORT 8765
#define CLIENT_PORT 9876
#define PACKET_SIZE 1024

/**
 * @brief Represents the server side of the network.
 *
 */
typedef struct
{
    /** Socket object */
    int sockfd, sockfd_b;

    /** IPv4 socket address */
    struct sockaddr_in servaddr, cliaddr;

    /** Message len */
    socklen_t clilen;

    /** Buffere told hold message */
    char buffer[PACKET_SIZE];

    /** Time variables */
    char start_time[20], end_time[20];
} Server;

/**
 * @brief Initializes the server. Sets up the
 *
 * @param *server object
 */
void start_server(Server *server)
{
    if (server == NULL)
    {
        perror("ERROR: initializing Server\n");
        exit(EXIT_FAILURE);
    }

    /** Configure Server struct sockaddr_in */
    server->servaddr.sin_family = AF_INET;                   // IPv4
    server->servaddr.sin_port = SERVER_PORT;                 // Server port
    server->servaddr.sin_addr.s_addr = inet_addr(SERVER_IP); // Set to default interface
    server->clilen = sizeof(server->cliaddr);

    /** Create UDP socket */
    server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sockfd < 0)
    {
        perror("ERROR: opening socket\n");
        exit(EXIT_FAILURE);
    }

    /** Bind server address to socket */
    if (bind(server->sockfd, (struct sockaddr *)&server->servaddr, sizeof(server->servaddr)) < 0)
    {
        perror("ERROR: Bind failed\n");
        exit(EXIT_FAILURE);
    }

    server->sockfd_b = accept(server->sockfd, (struct sockaddr *)&server->cliaddr, &server->clilen);
}

/**
 * @brief Receives two sets of UDP packets from the client.
 *
 * @param server this object pointer
 * @return int 0 if recieved successful, -1 otherwise
 */
int receive_packet(Server *server)
{
    printf("Listening for pakcets...\n");

    socklen_t len = sizeof(server->cliaddr);
    ssize_t n = recvfrom(server->sockfd, (char *)server->buffer, PACKET_SIZE, 0, (struct sockaddr *)&server->cliaddr, &len);
    server->buffer[n] = '\0';
    printf("Client: %s\n", server->buffer);

    // send the response
    sendto(server->sockfd, "msg recieved", PACKET_SIZE, 0, (struct sockaddr *)&server->cliaddr, sizeof(server->cliaddr));
    return 0;
}

/**
 * @brief Writes incoming file to given filename from socket connection object.
 *
 * @param sockfd object
 * @param *file filename
 */
void write_file(const char *file, int sockfd)
{
    FILE *fp = fopen(file, "wb");
    if (file == NULL)
    {
        perror("ERROR: Couldn't write file\n");
        exit(EXIT_FAILURE);
    }

    // Buffer variable to hold data
    char buffer[PACKET_SIZE];
    ssize_t recieved; // updates bytes recieved
    while ((recieved = recv(sockfd, buffer, PACKET_SIZE, 0)) > 0)
    {
        fwrite(buffer, 1, recieved, fp);
    }

    if (recieved < 0)
    {
        perror("ERROR: reciving data from socket\n");
    }

    fclose(fp);
    printf("Successfull!\n");
}

int main()
{
    Server *server = (Server *)malloc(sizeof(Server));
    printf("Created Server\n");

    /** Initialize the server */
    start_server(server);

    // char *file = "config.json";
    // write_file(file, server->sockfd);

    /** Listen for the packets */
    printf("Listening for packets...\n");

    receive_packet(server);

    sleep(5);

    /** Close socket connection */
    close(server->sockfd);

    /** Release resources related to the server object */
    free(server);

    return 0;
}