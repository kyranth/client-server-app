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
class Server
{
private:
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[PACKET_SIZE];
    double start_time_low, end_time_low;

public:
    /**
     * @brief Construct a new Server objectConstructor method for initializing the server.
     *
     */
    Server()
    {
        // Create UDP socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            error("Error opening socket");

        // Initialize server address struct
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(SERVER_PORT);

        // Bind socket to server address
        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Error binding");

        clilen = sizeof(cli_addr);
    }

    /**
     * @brief Clean up the Server object. Destructor method for cleaning up resources used by the server.
     *
     */
    ~Server()
    {
        close(sockfd);
    }

    /**
     * @brief Receives two sets of UDP packets from the client.
     *
     */
    void receivePackets()
    {
        // Receive first packet train (all 0's)
        if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0)
            error("Error receiving first packet train");
        start_time_low = get_time(); // Record start time

        // Receive second packet train (random bits)
        if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen) < 0)
            error("Error receiving second packet train");
        end_time_low = get_time(); // Record end time
    }
};

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}
