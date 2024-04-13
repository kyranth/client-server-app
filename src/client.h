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
 * @brief Represents the client side of the network compression detection application
 *
 */
class Client
{
private:
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[PACKET_SIZE];

public:
    Client()
    {
        // Create UDP socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            error("Error opening socket");

        // Initialize server address struct
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
            error("Invalid address");
    }

    ~Client()
    {
        close(sockfd);
    }

    /**
     * @brief
     */
    void sendPackets()
    {
        // Send first packet train (all 0's)
        memset(buffer, 0, PACKET_SIZE);
        if (sendto(sockfd, buffer, PACKET_SIZE, 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Error sending first packet train");

        // Send second packet train (random bits)
        FILE *urandom = fopen("/dev/urandom", "r");
        fread(buffer, sizeof(char), PACKET_SIZE, urandom);
        fclose(urandom);

        if (sendto(sockfd, buffer, PACKET_SIZE, 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Error sending second packet train");
    }
};

/**
 * @brief Generates the given error message
 *
 * @param msg
 */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/**
 * @brief Get the time object
 *
 * @return double
 */
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}