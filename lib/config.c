#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"
#include "config.h"

/**
 * @brief Create configuration
 */
Config *createConfig()
{
	Config *config = malloc(sizeof(Config));
	if (config == NULL)
	{
		perror("Failed to allocate memory for Config");
		exit(1);
	}

	// Allocate memory for server_ip_address
	*config->server_ip_address = (char *)malloc(16 * sizeof(char)); // Assuming IPv4 address, so 15 characters + null terminator

	return config;
}

/**
 * @brief Takes a filename and returns the cJSON obejct
 *
 * @param file pointer to a filename
 * @param config structure
 */
void setConfig(const char *file, Config *config)
{
	// open the file
	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		perror("Could not open config file\n");
		exit(1);
	}

	// read the file contents into a string
	char buffer[1024];
	fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);

	// parse the JSON data
	cJSON *json = cJSON_Parse(buffer);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			printf("Error: %s\n", error_ptr);
		}
		exit(1);
	}
	else
	{
		// /** Temp hardcoding them */
		// strcpy(config->server_ip_address, "127.0.0.1");
		// config->tcp_pre_probing_port = 7777;

		strcpy(config->server_ip_address,
			   cJSON_GetObjectItemCaseSensitive(json, "server_ip_address")->valuestring);
		config->tcp_pre_probing_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_pre_probing_port")->valueint;
		config->udp_source_port = cJSON_GetObjectItemCaseSensitive(json, "udp_source_port")->valueint;
		config->udp_destination_port = cJSON_GetObjectItemCaseSensitive(json, "udp_destination_port")->valueint;
		config->tcp_head_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_head_syn_port")->valueint;
		config->tcp_tail_syn_port = cJSON_GetObjectItemCaseSensitive(json, "tcp_tail_syn_port")->valueint;
		*config->udp_payload_size = cJSON_GetObjectItemCaseSensitive(json, "udp_payload_size")->valueint;
		*config->inter_measurement_time = cJSON_GetObjectItemCaseSensitive(json, "inter_measurement_time")->valueint;
		config->num_udp_packets = cJSON_GetObjectItemCaseSensitive(json, "num_udp_packets")->valueint;
		config->udp_packet_ttl = cJSON_GetObjectItemCaseSensitive(json, "udp_packet_ttl")->valueint;
	}

	// delete the JSON object
	cJSON_Delete(json);
}