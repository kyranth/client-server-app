#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "server.h"

#define THRESHOLD 200

/**
 * @brief Get the time
 *
 */
void get_time(char *time_str)
{
    time_t rawtime;
    struct tm *timeinfo;

    // Get current time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format time string
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

int main()
{
    Client *client = (Client *)malloc(sizeof(Client));
    printf("Created Client");

    Server *server = (Server *)malloc(sizeof(Server));
    printf("Created Server");

    /** Initialize the client and server */
    start_client(client);
    start_server(server);

    /** Listen for the packets */
    printf("Listening for packets...");
    int recv = receive_packet(server);
    if (recv < 0)
    {
        printf("Error: Counldn't send packet");
    }
    else
    {
        get_time(server->end_time);
    }

    /** Send the first packet train */
    int send = send_packet(client, "Hello");
    if (send < 0)
    {
        printf("Error: Counldn't send packet");
    }
    else
    {
        get_time(server->start_time);
        printf("Msg sent at %s", server->start_time);
    }

    /** TODO Send second packet train

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

    */

    close(client->sockfd);
    close(server->sockfd);

    return 0;
}