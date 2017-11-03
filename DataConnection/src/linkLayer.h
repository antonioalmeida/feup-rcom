#pragma once

#include <termios.h>
#include "Utilities.h"

void atende(int signal);
void setAlarm();
void stopAlarm();

#define RANDOM_ER_PROB 0

#define SEND 0
#define RECEIVE 1

#define C_SET 0x03
#define C_UA 0x07
#define C_RR 0x05
#define C_REJ 0x01
#define C_DISC 0x0B

#define SET 0
#define UA 1
#define RR 2
#define REJ 3
#define DISC 4

#define SU_MESSAGE 0
#define I_MESSAGE 1
#define INV_MESSAGE -1

#define SU_MESSAGE_SIZE 5*sizeof(char)
#define I_MESSAGE_BASE_SIZE 6*sizeof(char)

typedef enum {
	START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP
} machineState;

typedef struct {
	int fd;
	char port[20];
	int mode;
	int baudRate;
	int messageDataSize;
	unsigned int ns;
	unsigned int timeout;
	unsigned int numTries;
	char frame[MAX_SIZE];
	struct termios oldtio, newtio;

	int sentMessages;
	int receivedMessages;
	int timeouts;
	int numSentRR;
	int numReceivedRR;
	int numSentREJ;
	int numReceivedREJ;
} LinkLayer;

void initLinkLayer(const char* port, int mode, int baudrate, int messageDataSize, int timeout, int numRetries);

int llopen(const char* port, int mode);
int llwrite(const unsigned char* buf, unsigned int bufSize);
int llread(unsigned char** message);
int llclose(int mode);

void sendSUMessage(int command);
void sendIMessage(const unsigned char* buf, unsigned int bufSize);
unsigned char* readMessage(int* type, int* hasBCC2Error);
int messageIs(unsigned char* msg, int msgType, int command);
int getMessageSequenceNumber(unsigned char* msg, int type);
int getIMessageSize(unsigned char* msg);

unsigned int stuff(unsigned char** buf, unsigned int bufSize);
unsigned int destuff(unsigned char** buf, unsigned int bufSize);
unsigned char getBCC2(const unsigned char* buf, unsigned int size);
