#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Buffers sizes
#define MAX_INFO 200
#define MAX_MSG 2000
#define MAX_CPU_INFO 10

// Get CPU info
int getCPUInfo(char *cpuName)
{		
	FILE *file;

	file = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F \": \" '{print $2}'", "r");

	if (file == NULL) exit(1);

	fgets(cpuName, MAX_INFO, file);
	cpuName[strlen(cpuName)-1] = '\0';
	pclose(file);
	
	return 0;
}

// Get hostname
int getHostname(char *hostname)
{
	FILE *file;

	file = popen("cat /proc/sys/kernel/hostname", "r");

	if (file == NULL) exit(1);

	fgets(hostname, MAX_INFO, file);
	hostname[strlen(hostname)-1] = '\0';
	pclose(file);
	
	return 0;
}

// Convert string to unsigned long long int
void stringToLongLongInt(char *cpuName, unsigned long long int *idle, unsigned long long int *total)
{
	char tmp[100] = "\0";
	char *eptr;
	int pos = 0;

	for (unsigned int i = 5; i < strlen(cpuName); i++)
	{
		if (cpuName[i] == ' ' || cpuName[i] == '\n')
		{
			if (pos == 3 || pos == 4)
			{
				*idle += strtoll(tmp, &eptr, 10);
				*total += strtoll(tmp, &eptr, 10);
			}
			else if (pos <= 7)
			{
				*total += strtoll(tmp, &eptr, 10);
			}
			memset(tmp, '\0', 100);
			pos++;	
		}	
		else
		{
			strncat(tmp, &cpuName[i], 1);
			strcat(tmp, "\0");
		}
	}	
}

// Calculate CPU usage
int getCPUusage(char *cpuUsage)
{
	char cpuInfo[MAX_INFO+1];
	char cpuInfoPrev[MAX_INFO+1];
	FILE *file;

	file = popen("cat /proc/stat | grep cpu | head -n -1" , "r");
	if (file == NULL) exit(1);

	fgets(cpuInfoPrev, MAX_INFO, file);
	pclose(file);

	sleep(1);
	
	file = popen("cat /proc/stat | grep cpu | head -n -1" , "r");
	if (file == NULL) return 1;

	fgets(cpuInfo, MAX_INFO, file);
	pclose(file);

	unsigned long long int idle = 0;
	unsigned long long int prevIdle = 0;
	unsigned long long int total = 0;
	unsigned long long int prevTotal = 0;

	stringToLongLongInt(cpuInfoPrev, &prevIdle, &prevTotal);
	stringToLongLongInt(cpuInfo, &idle, &total);
	
	memset(cpuUsage, '\0', MAX_CPU_INFO);
	sprintf(cpuUsage, "%.2lf%%", (total - prevTotal - (idle - prevIdle))/(double)(total-prevTotal)*100);
		
	return 0;
}


int main(int argc, char *argv[])
{
	// Check number of arguments
	if (argc < 2)
	{
		fprintf(stderr, "ERROR: No port number!\n");
		exit(1);
	}
	
	// Port number, hostname, cpu-name
	int port = atoi(argv[1]);
	char hostname[MAX_INFO+1];
	char cpuName[MAX_INFO+1];
	char cpuUsage[MAX_CPU_INFO];

	getCPUInfo(cpuName);
	getHostname(hostname);
	
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mySocket <= 0)
	{
		fprintf(stderr, "ERROR: Cannot open new socket!\n");
		exit(1);
	}
	
	// Set options on socket
	const int enabled = 1;
	int err = setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &enabled, sizeof(enabled));
	if (err < 0)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket\n");
		exit(1);
	}
	
	// Address structure
	struct sockaddr_in mySocketAddr;
	mySocketAddr.sin_family = AF_INET;
	mySocketAddr.sin_addr.s_addr = INADDR_ANY;
	mySocketAddr.sin_port = htons(port);
	
	// Binding
	err = bind(mySocket, (struct sockaddr *)&mySocketAddr, sizeof(mySocketAddr));
	if (err < 0) 
	{
		fprintf(stderr, "ERROR: Binding!\n");
		exit(1);
	}

	// Listening for connections on a socket
	err = listen(mySocket, 5);
	if (err == -1)
	{
		fprintf(stderr, "ERROR: Cannot listen for connections on a socket!\n");
		exit(1);
	}
	
	char clientMessage[MAX_MSG+1];
	char goodHeader[100] = "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain;\r\n\r\n%s";
	char messageToSend[MAX_MSG+1] = "\0";
	int length = sizeof(mySocketAddr);
	
	while (1)
	{
		// Client Socket
		int clientSocket = accept(mySocket, (struct sockaddr *)&mySocketAddr, (socklen_t *)&length);
		if (clientSocket <= 0)
		{
			fprintf(stderr, "Accept connection failed\n");
			exit(1);
		}
		err = recv(clientSocket, clientMessage, MAX_MSG, 0);

		// Requests - hostname, load, cpu-name, bad request
		if (strncmp(clientMessage, "GET /hostname ", 14) == 0) 
		{
			sprintf(messageToSend, goodHeader, strlen(hostname), hostname);
		}
		else if (strncmp(clientMessage, "GET /load ", 10) == 0) 
		{
			getCPUusage(cpuUsage);
			sprintf(messageToSend, goodHeader, strlen(cpuUsage), cpuUsage);
		}
		else if (strncmp(clientMessage, "GET /cpu-name ", 14) == 0)
		{	
			sprintf(messageToSend, goodHeader, strlen(cpuName), cpuName);
		}
		else
		{
			sprintf(messageToSend, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\nContent-Length: 11\r\n\r\nBad Request");
		}
		
		// Send message to client
		write(clientSocket, messageToSend, strlen(messageToSend));
		// Close connection with client
		close(clientSocket);
		// Clear messageToSend buffer
		memset(messageToSend, '\0', MAX_MSG+1);
	}

	close(mySocket);
	return 0;	
}
