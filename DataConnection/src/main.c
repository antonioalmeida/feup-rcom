#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "ApplicationLayer.h"
#include "Utilities.h"

void start();

int main(int argc, char** argv) {
	start();
	return 0;
}

void start() {
	printf("Is this the transmitter (0) or the receiver (1)? ");
	int mode = getIntegerInRange(0, 1);
	printf("\n");

	printf("Choose the baudrate (1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200): ");
	int baudrate = getBaudrate();
	printf("\n");

	printf("Choose the maximum size for a data packet: ");
	int messageDataMaxSize = getIntegerInRange(1, 65535);
	printf("\n");

	printf("Choose maximum number of retries before aborting: ");
	int numRetries = getIntegerInRange(0, 10);
	printf("\n");

	printf("Choose the timeout value: ");
	int timeout = getIntegerInRange(1, 10);
	printf("\n");

	printf("Choose the port (x in /dev/ttySx) to be used: ");
	int portNum = getIntegerInRange(0, 9);

	char port[20];
	sprintf(port, "/dev/ttyS%d", portNum);
	printf("Using port: %s\n", port);
	printf("\n");

	char file[MAX_SIZE];
	if(mode == SEND){
		printf("Filename: ");
		scanf("%s", file);
		printf("\n");
	}

	// Initialize random seed
	srand((unsigned) time(NULL));

	initApplicationLayer(port, mode, baudrate, messageDataMaxSize, numRetries, timeout, file);
}
