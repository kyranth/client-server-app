#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define SERVER_PORT 8080
#define SIZE 1024

void p_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void process_config(int connfd)
{
    char buffer[SIZE];
    while (recv(connfd, buffer, SIZE, 0) > 0)
    {
        printf("Received: %s", buffer);
    }
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("Socket creation failed");
    }

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    // Bind socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Bind failed");
    }

    // Listen for connections
    if (listen(sockfd, 5) < 0)
    {
        p_error("Listen failed");
    }

    // Accept incoming connection
    len = sizeof(cliaddr);
    if ((connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
    {
        p_error("Accept failed");
    }

    // Pre-Probing Phase: Process config file
    process_config(connfd);

    // Probing Phase: Receive packet trains
    // (Here you would receive and process the UDP packets)
    // For the sake of simplicity, let's assume it's done elsewhere

    // Post-Probing Phase: Send findings
    // send(connfd, "Compression detected!", strlen("Compression detected!"), 0);

    // Close TCP connection
    close(connfd);
    close(sockfd);

    return 0;
}
