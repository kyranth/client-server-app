#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "client.h"
#include "server.h"

/**
 * @brief Get the time
 *
 * @return double time
 */
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

int main()
{
    Client *client;
    Server *server;

    /** Initialize the client and server */
    start_client(client);
    start_server(server);

    /** Send the first packet train */
    int send = send_packet(client, "Hello");
    if (send < 0)
    {
        printf("Error: Counldn't send packet");
    }

    /** Receive the packets */
    int recv = receive_packet(server);
    if (recv < 0)
    {
        printf("Error: Counldn't send packet");
    }

    double time_diff_low = server->end_time - server->start_time;

    printf("Time difference for low compression: %.2f ms\n", time_diff_low);

    // Check for compression
    if (time_diff_low > THRESHOLD)
    {
        printf("Compression detected!\n");
    }
    else
    {
        printf("No compression was detected.\n");
    }

    return 0;
}