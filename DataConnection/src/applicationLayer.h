#pragma once

#define I_PACKAGE_DATA 1
#define I_PACKAGE_START 2
#define I_PACKAGE_END 3

#define PARAM_FILE_SIZE 0
#define PARAM_FILE_NAME 1

#define SEND 0
#define RECEIVE 1

typedef struct {
	int fd;
	int mode;
	char* fileName;
	int maxSize;
} ApplicationLayer;

int initApplicationLayer(const char* port, int mode, int baudrate, int messageDataMaxSize, int numRetries, int timeout, char* file);
int getFileSize(char* filePath);
int startTransfer();
int sendData();
int receiveData();
int sendControlPackage(int C, char* fileSize, char* fileName);
int receiveControlPackage(int* ctrl, int* fileLength, char** fileName);
int sendDataPackage(int N, const char* buf, unsigned int length);
int receiveDataPackage(int* N, char** buf, unsigned int* length);
