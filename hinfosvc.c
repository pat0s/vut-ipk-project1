#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Get CPU info
int getCPUInfo(char *cpuName)
{		
	FILE *file;

	file = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F \": \" '{print $2}'", "r");

	if (file == NULL) return 1;

	fgets(cpuName, 200, file);
	//printf("%s", cpuName);

	pclose(file);
	
	return 0;
}

// Get hostname
int getHostname(char *hostname)
{
	FILE *file;

	file = popen("cat /proc/sys/kernel/hostname", "r");

	if (file == NULL) return 1;

	fgets(hostname, 200, file);
	//printf("%s", hostname);

	pclose(file);
	
	return 0;
}

// TODO
int getCPUusage()
{
	char cpuName[201];
	FILE *file;

	file = popen("cat /proc/stat", "r");

	if (file == NULL) return 1;

	fgets(cpuName, 200, file);
	printf("%s", cpuName);

	pclose(file);
	
	return 0;
}


int main(int argc, char *argv[])
{
	// Check number of arguments
	if (argc < 2)
	{
		fprintf(stderr, "ERROR: No port number!\n");
		return 1;
	}
	
	// Port number, hostname, cpu-name
	int port = atoi(argv[1]);
	char hostname[200];
	char cpuName[200];

	getCPUInfo(cpuName);
	getHostname(hostname);
	getCPUusage();
	
	const int enabled = 1;
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mySocket == -1)
	{
		fprintf(stderr, "ERROR: Cannot open new socket!\n");
		return 1;
	}
	
	int err = setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &enabled, sizeof(enabled));
	if (err == -1)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket\n");
		return 1;
	}
	
	struct sockaddr_in mySocketAddr;
	mySocketAddr.sin_family = AF_INET;
	mySocketAddr.sin_addr.s_addr = INADDR_ANY;
	mySocketAddr.sin_port = htons(port);
	
	err = bind(mySocket, (struct sockaddr *)&mySocketAddr, sizeof(mySocketAddr));
	if (err == -1) 
	{
		fprintf(stderr, "ERROR: Binding!");
		return 1;
	}

	// TODO - second argument = int backLog -> defins the maximum
	// length to which the queue of pending connections for socket
	// may grow
	err = listen(mySocket, 5);
	if (err == -1)
	{
		fprintf(stderr, "ERROR: Cannot listen for connections on a socket!");
		return 1;
	}
	
	char clientMessage[2001];
	// TODO don't know if ";" is necessary behind text/plain 
	char goodHeader[50] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;\r\n\r\n";
	char messageToSend[501] = "\0";
	int length = sizeof(mySocketAddr);
	
	while (1)
	{
		// Client Socket
		int clientSocket = accept(mySocket, (struct sockaddr *)&mySocketAddr, (socklen_t *)&length);
		if (clientSocket == -1)
		{
			fprintf(stderr, "Accept connection failed\n");
			return 1;
		}
		err = recv(clientSocket, clientMessage, 2000, 0);

		// hostname, load, cpu-name
		if (strncmp(clientMessage, "GET /hostname", 13) == 0) 
		{
			strcat(messageToSend, goodHeader);
			strcat(messageToSend, hostname);
		}
		else if (strncmp(clientMessage, "GET /load", 9) == 0) 
		{
			strcat(messageToSend, goodHeader);
			strcat(messageToSend, "Usage\0");
		}
		else if (strncmp(clientMessage, "GET /cpu-name", 13) == 0)
		{	
			strcat(messageToSend, goodHeader);
			strcat(messageToSend, cpuName);
		}
		else
		{
			strcat(messageToSend, """HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\nBad Request");	
		}
		
		// Write message to client
		write(clientSocket, messageToSend, strlen(messageToSend));
		// Close connection with client
		close(clientSocket);
		// Clear messageToSend buffer
		memset(messageToSend, '\0', 501);
	}

	close(mySocket);
	return 0;	
}
