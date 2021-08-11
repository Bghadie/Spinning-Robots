#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6000

int main() {
	//declare all variables
	int clientSocket, addrSize, bytesReceived;
	struct sockaddr_in serverAddr;
	unsigned char buffer[1];
  // Register with the server
	clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

	addrSize = sizeof(serverAddr);
	//store then send the stop command
	buffer[0] = STOP;
	
	sendto(clientSocket, buffer, 1, 0, 
	(struct sockaddr *)&serverAddr, addrSize);
	//close the socket 
	close(clientSocket);
	
	
}
