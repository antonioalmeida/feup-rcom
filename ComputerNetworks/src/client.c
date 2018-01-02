#include "client.h"

int connectSocket(const char* ip, int port) {
	int sockfd;
	struct sockaddr_in server_addr;

	// server address handling
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

	// open an TCP socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect to the server
	if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))< 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

/*
 * Connect socket (control socket) to ftp server
 * Receive a reply from the ftp server (code : 220).
 */
int connectServer(FTPInfo* client, const char* ip, int port) {
	char response[1024];

	if ((client->controlSocketFd = connectSocket(ip, port)) < 0) {
		printf("ERROR: Cannot connect socket.\n");
		return -1;
	}

	client->dataSocketFd = 0;

	if (readMessage(client, response, sizeof(response)) == -1) {
		printf("ERROR: readMessage failed.\n");
		return -1;
	}

	int reply_state = handleFTPReplyCode(response);

	if(reply_state != CONTINUE) {
		printf("ERROR: Server is not ready.\n");
		return -1;
	}

	return 0;
}

/*
 * Send login to the ftp server using the command USER and wait for confirmation (331)
 * Send password using the command PASS and wait for confirmation (230).
 */
int login(FTPInfo* client, const char* username, const char* password) {
	char message[1024];

	// Sends username
	sprintf(message, "USER %s\r\n", username); //'\r' is to simulate 'ENTER'

	if(sendCommand(client, message) != 0) {
		printf("ERROR: USER command received 5XX code.\n");
		return -1;
	}

	// cleaning buffer
	memset(message, 0, sizeof(message));

	// Sends password
	sprintf(message, "PASS %s\r\n", password);

	if(sendCommand(client, message) != 0) {
		printf("ERROR: PASS command received 5XX code.\n");
		return -1;
	}

	return 0;
}


int download(FTPInfo* client, const char* fileName) {

	if(requestRETR(client, fileName) == -1) {
		printf("ERROR: sendRETR failed.\n");
		return -1;
	}

	FILE* file = fopen(fileName, "wb");

	if (file == NULL) {
		printf("ERROR: Could not create the file.\n");
		return -1;
	}

	char fileBuf[1024];
	int nBytesRead = 0;

	while ((nBytesRead = read(client->dataSocketFd, fileBuf, sizeof(fileBuf)))) {
		if (nBytesRead < 0) {
			printf("ERROR: Nothing was received from data socket fd.\n");
			return -1;
		}

		if ((nBytesRead = fwrite(fileBuf, nBytesRead, 1, file)) < 0) {
			printf("ERROR: Cannot write data in file.\n");
			return -1;
		}
	}

	fclose(file);
	close(client->dataSocketFd);

	return 0;
}

/*
 * Send the QUIT command and wait for reply (221)
 */
int disconnectServer(FTPInfo* client) {
	char message[1024];

	sprintf(message, "QUIT\r\n");

	if(sendCommand(client, message) != 0) {
		printf("ERROR: QUIT command received 5XX code.\n");
		return -1;
	}

	close(client->controlSocketFd);

	return 0;
}

int readMessage(FTPInfo* client, char* message, size_t messageSize) {
	FILE* fd = fdopen(client->controlSocketFd, "r");

	do {
		memset(message, 0, messageSize);
		message = fgets(message, messageSize, fd);
		if(message == NULL){
			printf("ERROR: Cannot read the message.\n");
			return -1;
		}
		printf("%s", message);
	} while (!('1' <= message[0] && message[0] <= '5') || message[3] != ' ');

	return 0;
}

int sendMessage(FTPInfo* client, const char* message, size_t messageSize) {
	int nBytesRead = 0;

	if ((nBytesRead = write(client->controlSocketFd, message, messageSize)) <= 0) {
		printf("ERROR: Message wasn't send.\n");
		return -1;
	}

	//printf("Bytes send: %d\n", nBytesRead);
	printf("Info: %s\n", message);

	return 0;
}

int setCWD(FTPInfo* client, const char* path) {
	char message[1024];

	if(strlen(path) <= 0)
		return 0;

	sprintf(message, "CWD %s\r\n", path);

	if(sendCommand(client, message) != 0) {
		printf("download: CWD command received 5XX code.\n");
		return -1;
	}

	return 0;
}

/*
 * Send command PASV, and waits for a reply that gives an IP address and a port
 * Parse this message,
 * Connect a second socket (a data socket) with the given configuration.
 */
int setPassiveMode(FTPInfo* client) {
	char message[1024] = "PASV\r\n";

	if(sendCommand(client, message) != 0) {
		printf("ERROR: PASV command received 5XX code.\n");
		return -1;
	}

	// Parses the message received
	int ipPart1, ipPart2, ipPart3, ipPart4, portPart1, portPart2;

	if ((sscanf(message, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
			&ipPart1, &ipPart2, &ipPart3, &ipPart4, &portPart1, &portPart2)) < 0) {
		printf("ERROR: Cannot process information to calculating port.\n");
		return -1;
	}

	char ip[1024];

	// Builds IP
	if (sprintf(ip, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4) < 0) {
		printf("ERROR: Cannot form ip address.\n");
		return -1;
	}

	// Builds Port
	int port = portPart1 * 256 + portPart2;

	printf("IP: %s\nPORT: %d\n", ip, port);

	if ((client->dataSocketFd = connectSocket(ip, port)) < 0) {
		printf("ERROR: Cannot connect to socket on PASV.\n");
		return -1;
	}

	return 0;
}

/*
 * Send the RETR command with the file path
 */
int requestRETR(FTPInfo* client, const char* fileName) {
	char message[1024];
	char response[1024];

	sprintf(message, "RETR %s\r\n", fileName);

	if (sendMessage(client, message, strlen(message)) == -1) {
		printf("ERROR: sendMessage failed.\n");
		return -1;
	}

	if (readMessage(client, response, sizeof(response)) == -1) {
		printf("ERROR: readMessage failed.\n");
		return -1;
	}

	return 0;
}

int sendCommand(FTPInfo* client, char* command) {

	int reply_state;
	char response[1024];

	/* Repeat until reply code signals completion */
	do {
		/* Write command and read response */
		if (sendMessage(client, command, strlen(command)) == -1) {
			printf("ERROR: sendMessage failed.\n");
			return -1;
		}

		if (readMessage(client, response, sizeof(response)) == -1) {
			printf("ERROR: readMessage failed.\n");
			return -1;
		}

		reply_state = handleFTPReplyCode(response);

		/* Handle reply code */
		if(reply_state == ABORT) return -1;
		else if(reply_state == WAIT_REPLY) {
			if (readMessage(client, response, sizeof(response)) == -1) {
				printf("ERROR: readMessage failed.\n");
				return -1;
			}
			reply_state = handleFTPReplyCode(response);
			if(reply_state == ABORT) return -1;
		}

	} while(reply_state != CONTINUE);

	memcpy(command, response, sizeof(response));
	return 0;
}

int handleFTPReplyCode(char* reply_code) {

	int result = parseFTPReplyCode(reply_code);
	int hundreds = (result / 100) % 100;

	switch(hundreds) {
	case R1XX:
		return WAIT_REPLY;
		break;
	case R2XX:
		return CONTINUE;
		break;
	case R3XX:
		return CONTINUE;
		break;
	case R4XX:
		return REPEAT;
		break;
	case R5XX:
		return ABORT;
		break;
	default:
		return ABORT;
		break;
	}
}

int parseFTPReplyCode(char* reply) {

	/* Copy first 3 values of reply message which should be the reply code */
	char code[4];
	strncpy(code, reply, 3);
	code[3] = '\0';

	unsigned long number = strtoul(code, NULL, 10);

	return number;
}
