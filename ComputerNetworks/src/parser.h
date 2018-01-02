#pragma once

#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_SIZE 256

typedef struct {
	char user[MAX_SIZE];
	char password[MAX_SIZE];
	char host[MAX_SIZE];
	char ip[MAX_SIZE];
	char path[MAX_SIZE];
	char filename[MAX_SIZE];
	int port;
} URLInfo;

int parseURL(URLInfo* url, const char* str);
int getIpByHost(URLInfo* url);
int getSubstring(const char* str, int start, char delimiter, int* substringLength);
