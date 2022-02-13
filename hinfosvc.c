#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Get CPU info
int getCPUInfo(char *cpuName)
{		
	FILE *file;

	file = popen("cat /proc/cpuinfo | grep \"model name\" | head -n 1 | awk -F \": \" '{print $2}'", "r");

	if (file == NULL) exit(1);

	fgets(cpuName, 200, file);
	pclose(file);
	
	return 0;
}

// Get hostname
int getHostname(char *hostname)
{
	FILE *file;

	file = popen("cat /proc/sys/kernel/hostname", "r");

	if (file == NULL) exit(1);

	fgets(hostname, 200, file);
	pclose(file);
	
	return 0;
}

// Convert strings to long long int
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

int getCPUusage(char *cpuUsage)
{
	char cpuName[201];
	char cpuNamePrev[201];
	FILE *file;

	file = popen("cat /proc/stat | grep cpu | head -n -1" , "r");
	if (file == NULL) exit(1);

	fgets(cpuNamePrev, 200, file);
	pclose(file);

	sleep(1);
	
	file = popen("cat /proc/stat | grep cpu | head -n -1" , "r");
	if (file == NULL) return 1;

	fgets(cpuName, 200, file);
	pclose(file);

	unsigned long long int idle = 0;
	unsigned long long int prevIdle = 0;
	unsigned long long int total = 0;
	unsigned long long int prevTotal = 0;

	stringToLongLongInt(cpuNamePrev, &prevIdle, &prevTotal);
	stringToLongLongInt(cpuName, &idle, &total);
	
	memset(cpuUsage, '\0', 10);
	sprintf(cpuUsage, "%.2lf%%", (total - prevTotal - (idle - prevIdle))/(double)(total-prevTotal)*100);
		
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
	char cpuUsage[10];

	getCPUInfo(cpuName);
	getHostname(hostname);
	
	const int enabled = 1;
	int mySocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mySocket == -1)
	{
		fprintf(stderr, "ERROR: Cannot open new socket!\n");
		exit(1);
	}
	
	int err = setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &enabled, sizeof(enabled));
	if (err == -1)
	{
		fprintf(stderr, "ERROR: Cannot set options on socket\n");
		exit(1);
	}
	
	struct sockaddr_in mySocketAddr;
	mySocketAddr.sin_family = AF_INET;
	mySocketAddr.sin_addr.s_addr = INADDR_ANY;
	mySocketAddr.sin_port = htons(port);
	
	// Binding
	err = bind(mySocket, (struct sockaddr *)&mySocketAddr, sizeof(mySocketAddr));
	if (err == -1) 
	{
		fprintf(stderr, "ERROR: Binding!");
		exit(1);
	}

	// Listening
	err = listen(mySocket, 5);
	if (err == -1)
	{
		fprintf(stderr, "ERROR: Cannot listen for connections on a socket!");
		exit(1);
	}
	
	char clientMessage[2001];
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
			exit(1);
		}
		err = recv(clientSocket, clientMessage, 2000, 0);

		// hostname, load, cpu-name
		if (strncmp(clientMessage, "GET /hostname ", 14) == 0) 
		{
			strcat(messageToSend, goodHeader);
			strcat(messageToSend, hostname);
		}
		else if (strncmp(clientMessage, "GET /load ", 10) == 0) 
		{
			strcat(messageToSend, goodHeader);
			getCPUusage(cpuUsage);
			strcat(messageToSend, cpuUsage);
		}
		else if (strncmp(clientMessage, "GET /cpu-name ", 14) == 0)
		{	
			strcat(messageToSend, goodHeader);
			strcat(messageToSend, cpuName);
		}
		else
		{
			sprintf(messageToSend, "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain;\r\n\r\nBad Request");	
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
