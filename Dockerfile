# Start your image base image
FROM ubuntu:latest

# Dependent on machines
WORKDIR /workspaces/client-server-app

# Update and install build tools
RUN apt-get update \ 
	&& apt install build-essentials \ 
	&& apt install git
