#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define R1XX		   1															//FTP Reply code hundreds digit
#define R2XX		   2															//FTP Reply code hundreds digit
#define R3XX		   3															//FTP Reply code hundreds digit
#define R4XX		   4															//FTP Reply code hundreds digit
#define R5XX		   5	 														//FTP Reply code hundreds digit

#define WAIT_REPLY 10
#define CONTINUE   11
#define REPEAT     12
#define ABORT      13

typedef struct {
	int controlSocketFd; // file descriptor to control socket
	int dataSocketFd; // file descriptor to data socket
} FTPInfo;

int connectSocket(const char *ip, int port);
int connectServer(FTPInfo* client, const char* ip, int port);
int login(FTPInfo* client, const char* username, const char* password);
int download(FTPInfo* client, const char* fileName);
int disconnectServer(FTPInfo* client);

int readMessage(FTPInfo* client, char* message, size_t messageSize);
int sendMessage(FTPInfo* client, const char* message, size_t messageSize);
int handleFTPReplyCode(char* reply_code);
int parseFTPReplyCode(char* reply);
int sendCommand(FTPInfo* client, char* command);

int setCWD(FTPInfo* client, const char* path);
int setPassiveMode(FTPInfo* client);
int requestRETR(FTPInfo* client, const char* fileName);
