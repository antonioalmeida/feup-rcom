#include <stdio.h>
#include "parser.h"
#include "client.h"

static void printUsage(char* argv0);

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("WARNING: Wrong number of arguments.\n");
		printUsage(argv[0]);
		return -1;
	}

	///////////// URL PROCESS /////////////
	URLInfo* url = (URLInfo*) malloc(sizeof(URLInfo));
	url->port = 21;

	// start parsing argv[1] to URL components
	if (parseURL(url, argv[1]))
		return -1;

	// edit url ip by hostname
	if (getIpByHost(url)) {
		printf("ERROR: Cannot find ip to hostname %s.\n", url->host);
		return -1;
	}

	printf("\nThe IP received to %s was %s\n", url->host, url->ip);

	///////////// FTP CLIENT PROCESS /////////////

	FTPInfo* ftp = (FTPInfo*) malloc(sizeof(FTPInfo));;
	if(connectServer(ftp, url->ip, url->port) < 0){
		printf("ERROR: Failure connecting to FTP server control port.\n");
		return -1;
	}

	// Verifying username
	const char* user = strlen(url->user) ? url->user : "anonymous";

	// Verifying password
	char* password;

	if (strlen(url->password)) {
		password = url->password;
	} else {
		printf("You are now entering in anonymous mode.\n");
		password = "random";
	}

	// Sending credentials to server
	if (login(ftp, user, password)) {
		printf("ERROR: Cannot login user %s\n", user);
		return -1;
	}

	// Changing directory
	if (setCWD(ftp, url->path)) {
		printf("ERROR: Cannot change directory to the folder of %s\n",url->filename);
		return -1;
	}

	// Entry in passive mode
	if (setPassiveMode(ftp)) {
		printf("ERROR: Cannot entry in passive mode\n");
		return -1;
	}

	// Starting file transfer
	if(download(ftp, url->filename) < 0){
		printf("ERROR: Cannot download the file\n");
		return -1;
	}

	// Disconnecting from server
	if(disconnectServer(ftp) < 0){
		printf("ERROR: Cannot disconnect from server\n");
		return -1;
	}

	return 0;
}

void printUsage(char* argv0) {
	printf("\nUsage1 Normal: %s ftp://[<user>:<password>@]<host>/<url-path>\n",argv0);
	printf("Usage2 Anonymous: %s ftp://<host>/<url-path>\n\n", argv0);
}
