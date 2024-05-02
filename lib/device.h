#ifndef DEVICE_H
#define DEVICE_H

#include <stdio.h>
#include <stdlib.h>
#include "config.h" // Include the header file for the Config structure
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Structure definition for Device
typedef struct
{
	int sockfd, sockfd_b;
	struct sockaddr_in servaddr;
} Device;

/**
 * @brief Initializes the Device with the given Config.
 *
 * @param device Device structure to initialize.
 * @param config Config structure containing initialization data.
 */
void device_start(Device device, Config config);

/**
 * @brief Connects the Device to the server.
 *
 * @param device Device structure to connect.
 */
void device_connect(Device device);

#endif /* DEVICE_H */
