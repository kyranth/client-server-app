# Start your image base image
FROM ubuntu:latest

# Update and install build tools
RUN apt-get update \
	&& apt install git \
	&& apt install build-essential	
