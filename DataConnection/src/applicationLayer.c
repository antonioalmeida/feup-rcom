#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include "LinkLayer.h"
#include "Utilities.h"
#include "ApplicationLayer.h"

#define S_TO_MS 1e3
#define NS_TO_MS 1e-6

ApplicationLayer* al;

int initApplicationLayer(const char* port, int mode, int baudrate, int messageDataMaxSize, int numRetries, int timeout, char* file) {
	al = (ApplicationLayer*) malloc(sizeof(ApplicationLayer));

	initLinkLayer(port, mode, baudrate, messageDataMaxSize, numRetries, timeout);

	//Setup connection
	al->fd = llopen(port, mode);
	if (al->fd <= 0)
		return -1;

	al->mode = mode;
	al->fileName = file;
	al->maxSize = messageDataMaxSize;

	struct timespec starting_time_temp;
	if(clock_gettime(CLOCK_REALTIME, &starting_time_temp) == -1)
		printf("ERROR: %s\n", strerror(errno));
	double starting_time = (double)(starting_time_temp.tv_sec*S_TO_MS + starting_time_temp.tv_nsec*NS_TO_MS);

	startTransfer();

	struct timespec final_time_temp;
	if(clock_gettime(CLOCK_REALTIME, &final_time_temp) == -1)
		printf("ERROR : %s\n", strerror(errno));
	double final_time = (double)(final_time_temp.tv_sec*S_TO_MS + final_time_temp.tv_nsec*NS_TO_MS);

	printf("Transfer time: %.2f ms\n", final_time - starting_time);


	if (!llclose(al->mode)) {
		printf("ERROR: Serial port was not closed.\n");
		return -1;
	}


	return 1;
}

int getFileSize(char* filePath) {
	int size;
	struct stat temp;
	if(stat(filePath, &temp) != 0){
		printf("ERROR: Unable to get file size\n");
		return -1;
	}

	size = temp.st_size;
	return size;
}

int startTransfer() {
	switch (al->mode) {
	case RECEIVE:
		printf("Starting transfer as RECEIVER.\n");
		if(receiveData() <= 0)
			exit(-1);
		break;
	case SEND:
		printf("Starting transfer as TRANSMITTER.\n");
		if(sendData() <= 0)
			exit(-1);
		break;
	default:
		break;
	}

	return 1;
}

int sendData() {
	FILE* file = fopen(al->fileName, "rb");
	if (file == NULL) {
		printf("ERROR: Could not open file to be sent.\n");
		return -1;
	}

	// Get file size (using stat utility)
	int fileSize = getFileSize(al->fileName);
	if(fileSize < 0)
		return -1;
	char fileSizeBuf[sizeof(unsigned int) * 3 + 2];
	snprintf(fileSizeBuf, sizeof(fileSizeBuf), "%d", fileSize);

	if (!sendControlPackage(I_PACKAGE_START, fileSizeBuf, al->fileName))
		return -1;

	char* fileBuf = malloc(al->maxSize);

	printf("Starting file data transfer\n");

	unsigned int nBytesRead = 0, i = 0;
	while ((nBytesRead = fread(fileBuf, sizeof(char), al->maxSize, file)) > 0) {
		if (!sendDataPackage((i++) % 255, fileBuf, nBytesRead)) {
			free(fileBuf);
			return -1;
		}

		memset(fileBuf, 0, al->maxSize);
	}
	printf("\n\n");

	printf("Finished file data transfer\n");

	free(fileBuf);

	if (fclose(file) != 0) {
		printf("ERROR: Unable to close file.\n");
		return -1;
	}

	if (!sendControlPackage(I_PACKAGE_END, fileSizeBuf, al->fileName))
		return -1;

	return 1;
}

int receiveData() {
	int controlStart;
	int controlEnd;
	int fileSize;
	char* fileName = (char*)malloc(MAX_SIZE);
	bzero(fileName, MAX_SIZE); //Otherwise, random characters will be appended to fileName

	printf("Waiting for START control package.\n");
	if (!receiveControlPackage(&controlStart, &fileSize, &fileName))
		return -1;

	if (controlStart != I_PACKAGE_START) {
		printf("ERROR: Expected START control package, got control package number %d instead\n", controlStart);
		return -1;
	}

	FILE* outputFile = fopen(fileName, "wb");
	if (outputFile == NULL) {
		printf("ERROR: Could not create output file.\n");
		return -1;
	}

	printf("\n");
	printf("Created output file: %s\n", fileName);
	printf("Expected file size: %d (bytes)\n", fileSize);
	printf("\n");

	printf("Starting file data reception\n");

	int N = -1;
	unsigned int totalSizeRead = 0;
	while (totalSizeRead != fileSize) {
		int lastN = N;
		char* fileBuf = NULL; //Is initialized inside the function with the appropriate read length
		unsigned int length = 0;

		if (!receiveDataPackage(&N, &fileBuf, &length)) {
			printf("ERROR: Could not receive data package.\n");
			free(fileBuf);
			return -1;
		}

		if (N != 0 && lastN + 1 != N) {
			printf("ERROR: Expected sequence %d, received %d instead\n", lastN+1, N);
			free(fileBuf);
			return -1;
		}

		fwrite(fileBuf, sizeof(char), length, outputFile);
		free(fileBuf);

		totalSizeRead += length;
		printf("Read data package with %d bytes, %d/%d bytes read so far\n", length, totalSizeRead, fileSize);
	}

	printf("\nFinished file data reception\n");

	if (fclose(outputFile) != 0) {
		printf("ERROR: Closing output file.\n");
		return -1;
	}

	if (!receiveControlPackage(&controlEnd, 0, NULL))
		return -1;

	if (controlEnd != I_PACKAGE_END) {
		printf("ERROR: Expected START control package, got control package number %d instead\n", controlEnd);
		return -1;
	}

	return 1;
}

int sendControlPackage(int C, char* fileSize, char* fileName) {
	int packageSize = 5 + strlen(fileSize) + strlen(fileName); //Guaranteed at least always 5 bytes: C, T1, L1, T2, L2
	unsigned int index = 0;

	// Creating the package
	unsigned char package[packageSize];
	package[index++] = C;
	package[index++] = PARAM_FILE_SIZE;
	package[index++] = strlen(fileSize);
	unsigned int i;
	for (i = 0; i < strlen(fileSize); i++)
		package[index++] = fileSize[i];
	package[index++] = PARAM_FILE_NAME;
	package[index++] = strlen(fileName);
	for (i = 0; i < strlen(fileName); i++)
		package[index++] = fileName[i];

	if (C == I_PACKAGE_START) {
		printf("\nFile: %s\n", fileName);
		printf("Size: %s (bytes)\n\n", fileSize);
	}

	// Actually sending
	if (!llwrite(package, packageSize)) {
		printf("ERROR: Could not write control package.\n");
		free(package);
		return -1;
	}

	return 1;
}

int receiveControlPackage(int* controlPackageType, int* fileLength, char** fileName) {
	unsigned char* package;
	unsigned int packageSize = llread(&package);
	if (packageSize < 0) {
		printf("ERROR: Could not read from link layer while receiving control package.\n");
		return -1;
	}

	*controlPackageType = package[0];

	// END package just symbolizes transfer process is over, no need to process anything further
	if (*controlPackageType == I_PACKAGE_END)
		return 1;

	unsigned int i;
	unsigned int index = 1;
	unsigned int numBytes = 0;
	for (i = 0; i < 2; ++i) {
		int paramType = package[index++];

		if(paramType == PARAM_FILE_SIZE){
			numBytes = (unsigned int) package[index++];

			char* length = (char*)malloc(numBytes);
			memcpy(length, &package[index], numBytes);

			*fileLength = atoi(length);
			free(length);
		}
		else if(paramType == PARAM_FILE_NAME){
			numBytes = (unsigned char) package[index++];
			memcpy(*fileName, &package[index], numBytes);
		}
		else
			printf("Unrecognised file parameter, skipping\n");

		index += numBytes;
	}

	return 1;
}

int sendDataPackage(int N, const char* buffer, unsigned int length) {
	unsigned char C = I_PACKAGE_DATA;
	unsigned char L2 = length / 256;
	unsigned char L1 = length % 256;

	unsigned int packageSize = 4 + length; //4 bytes are from C, N, L2 and L1

	unsigned char* package = (unsigned char*) malloc(packageSize);
	package[0] = C;
	package[1] = N;
	package[2] = L2;
	package[3] = L1;
	memcpy(&package[4], buffer, length);

	// Actually send package
	if (!llwrite(package, packageSize)) {
		printf("ERROR: Could not write data package.\n");
		free(package);
		return -1;
	}

	free(package);

	return 1;
}

int receiveDataPackage(int* N, char** buf, unsigned int* length) {
	unsigned char* package;

	unsigned int packageSize = llread(&package);
	if (packageSize < 0) {
		printf("ERROR: Could not read data package.\n");
		return -1;
	}

	int C = package[0];
	*N = (unsigned char) package[1];
	int L2 = package[2];
	int L1 = package[3];

	if (C != I_PACKAGE_DATA) {
		printf("ERROR: Received package is not a data package.\n");
		return -1;
	}

	*length = (L2 << 8) + L1;
	*buf = (char*)malloc(*length);
	memcpy(*buf, &package[4], *length);
	free(package);
	return 1;
}
