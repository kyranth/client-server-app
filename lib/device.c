#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "device.h"
#include "config.h"
#include "config.c"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void device_start(Device device, Config config)
{
	// Create TCP socket
	if ((device.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		p_error("Socket creation failed");
	}

	// Set server address
	struct sockaddr_in addr = device.servaddr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(config.server_ip_address);
	addr.sin_port = htons(config.tcp_pre_probing_port);
}

void device_connect(Device device)
{
	struct sockaddr_in addr = device.servaddr;
	// Connect to server
	if (connect(device.sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		p_error("Connection Failed");
		exit(1);
	}
}