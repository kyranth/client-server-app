#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>

#define SERVER_IP "172.16.4.4"
#define SERVER_PORT 8080
#define SIZE 1024
#define PACKET_SIZE 1024
#define INTER_MEASUREMENT_TIME 1
#define THRESHOLD 100

void p_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_config(int sockfd)
{
    FILE *config_file;
    char buffer[SIZE];
    config_file = fopen("config.json", "r");
    if (config_file == NULL)
    {
        p_error("Error opening config file.");
    }
    while (fgets(buffer, SIZE, config_file) != NULL)
    {
        send(sockfd, buffer, strlen(buffer), 0);
    }
    fclose(config_file);
}

void send_packet_train(int sockfd, struct sockaddr_in servaddr, int n, int entropy)
{
    char packet[PACKET_SIZE];
    // struct timeval start_time, end_time;
    // gettimeofday(&start_time, NULL);
    for (int i = 0; i < n; ++i)
    {
        memset(packet, 0, PACKET_SIZE);
        *(uint16_t *)packet = htons(i); // Packet ID
        sendto(sockfd, packet, PACKET_SIZE, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
    // gettimeofday(&end_time, NULL);
    if (entropy)
        sleep(INTER_MEASUREMENT_TIME); // Wait before sending high entropy data
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    // struct timeval start_time_low, end_time_low, start_time_high, end_time_high;
    // double low_entropy_time, high_entropy_time;

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        p_error("Socket creation failed");
    }

    // Set server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        p_error("Connection Failed");
    }

    // Pre-Probing Phase: Send config file
    send_config(sockfd);

    // Probing Phase: Send packet trains
    // gettimeofday(&start_time_low, NULL);
    // send_packet_train(sockfd, servaddr, 10, 0); // Low entropy data
    // gettimeofday(&end_time_low, NULL);
    // gettimeofday(&start_time_high, NULL);
    // send_packet_train(sockfd, servaddr, 10, 1); // High entropy data
    // gettimeofday(&end_time_high, NULL);

    // Post-Probing Phase: Receive findings from server
    // char buffer[SIZE];
    // read(sockfd, buffer, SIZE);
    // printf("Server findings: %s\n", buffer);

    // Close TCP connection
    close(sockfd);

    // Calculate time differences
    // low_entropy_time = (end_time_low.tv_sec - start_time_low.tv_sec) * 1000.0 + (end_time_low.tv_usec - start_time_low.tv_usec) / 1000.0;
    // high_entropy_time = (end_time_high.tv_sec - start_time_high.tv_sec) * 1000.0 + (end_time_high.tv_usec - start_time_high.tv_usec) / 1000.0;

    // Check for compression
    // if ((high_entropy_time - low_entropy_time) > THRESHOLD)
    // {
    //     printf("Compression detected!\n");
    // }
    // else
    // {
    //     printf("No compression was detected.\n");
    // }

    return 0;
}
