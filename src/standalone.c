#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <client.h>
#include <server.h>

int main()
{
    Client client;
    Server server;

    client.sendPackets();
    server.receivePackets();

    double time_diff_low = server.getTimeDifference();

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