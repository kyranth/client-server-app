# Client-Server Application README
This is a client-server application developed in C programming language. The application facilitates the compression link and communication between a client and a server over TCP and UDP connections. The main functionality involves transferring a configuration file from the client to the server over TCP, followed by the exchange of low entropy and high entropy UDP packets, with subsequent data analysis and result transmission back to the client over TCP.

## Features
- Configuration File Transfer: The client reads a config.json file, which contains necessary parameters for the application's behavior.
TCP File Transfer: Initiates a TCP connection with the server and sends the config.json file.
- Server-Side File Handling: The server receives the file from the client and writes it to the file system. Then it reads the file into memory.
- UDP Packet Exchange:
	- Low Entropy Packets: The client sends *n* low entropy (all 0s) UDP packets to the server over a UDP connection.
	- High Entropy Packets: After the inter measurement time, the client sends *n* high entropy packets (generated from file) to the server.
- Packet Time Recording: The server records the time of the first packet and the last packet received for both low and high entropy packets.
- Delta Calculation: Calculates the time difference between receiving *n* low entropy packets and *n* high entropy packets.
- Result Transmission: The server sends the calculated delta to the client over a TCP connection.

## Configuration:
Ensure that the `config.json` and the `random_file` file exists in the client's directory and contains necessary parameters for the application.

Configuration File (config.json)
The `config.json` file should have the following structure:

```
{
    "server_ip_address": "REPLACE_WITH_YOUR_IP",
    "udp_source_port": 9876,
    "udp_destination_port": 8765,
    "tcp_head_syn_port": 9999,
    "tcp_tail_syn_port": 8888,
    "tcp_pre_probing_port": 7777,
    "tcp_post_probing_port": 6666,
    "udp_payload_size": 1000,
    "inter_measurement_time": 15,
    "num_udp_packets": 6000,
    "udp_packet_ttl": 255
}
```

## Usage
1. Setup two different Virtual Machines, one for Client and one Server.
2. Compilation: Compile the client and server source files using a C compiler (e.g., gcc).
- The make simplifies this process. Simply run the command below. However, compile each program on respective VMs.
	```
	make client
	```
	```
	make server
	```
- This compiles and generates executables for client and server application. 

	**Note:** *the cJSON library is include in the object files and compiles as one executable. Which is required to run the applications.*

3. **Execution** 

	Both program requires command line arguments

	- Client side, as you can see in the makefile, client takes the `config.json` file
		```
		make run
		```
		or
		```
		./client config
		```

	- Server side, executable and port number (Must be the same as config file)
		```
		./server 7777
		```
4. Docker was used to simply built the project. DO NOT use for testing purposes.

## Dependencies
Standard C libraries
Socket programming libraries for TCP and UDP communication
[cJSON](https://github.com/DaveGamble/cJSON) library for parsing the config file

# Limitations
- Since the probing phase consists of UDP connections, it may lose a couple of packets.
- Unfortunately the program can't run the part 2 standalone program due to time limitations, couldn't initiate connections. Although the `standalone.c` program has some implementations for RAW Sockets.
- The `lib` directory holds the libraries required for this project. Although, some custom headers file might exist there to spread the project over multiple files, it's not yet implemented due to time limitations.

# Findings 
The `client-server.pcap` file demonstrates the network compression link from this project over 12,0000 packet frames.

## License
This project is licensed under the MIT License - see the LICENSE file for details.
