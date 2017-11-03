#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "ApplicationLayer.h"
#include "LinkLayer.h"

LinkLayer* ll;

const int FLAG = 0x7E;
const int A = 0x03;
const int ESCAPE = 0x7D;
int alarmWentOff = 0;

void atende(int signal) {
	alarmWentOff = 1;
	ll->timeouts++;
	printf("Timeout. Retrying\n");

	alarm(ll->timeout);
}

void setAlarm() {
	struct sigaction action;
	action.sa_handler = atende;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	sigaction(SIGALRM, &action, NULL);

	alarmWentOff = 0;

	alarm(ll->timeout);
}

void stopAlarm() {
	struct sigaction action;
	action.sa_handler = NULL;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	sigaction(SIGALRM, &action, NULL);

	alarm(0);
}

void initLinkLayer(const char* port, int mode, int baudrate, int messageDataSize, int numRetries, int timeout) {
	ll = (LinkLayer*) malloc(sizeof(LinkLayer));

	strcpy(ll->port, port);
	ll->mode = mode;
	ll->baudRate = baudrate;
	ll->messageDataSize = messageDataSize;
	ll->ns = 0;
	ll->timeout = timeout;
	ll->numTries = 1 + numRetries;

	ll->sentMessages = 0;
	ll->receivedMessages = 0;
	ll->timeouts = 0;
	ll->numSentRR = 0;
	ll->numReceivedRR = 0;
	ll->numSentREJ = 0;
	ll->numReceivedREJ = 0;
}

int llopen(const char* port, int mode) {
	printf("Estabilishing connection\n");

	// Open serial port device for reading and writing and not as controlling
	// tty because we don't want to get killed if linenoise sends CTRL-C.
	ll->fd = open(port, O_RDWR | O_NOCTTY);
	if(ll->fd < 0){
		printf("ERROR: Couldn't open serial port\n");
		return -1;
	}

	//Set new termios structure, saving the old one to undo the change in the end
	tcgetattr(ll->fd, &ll->oldtio);

	bzero(&ll->newtio, sizeof(ll->newtio));
	ll->newtio.c_cflag = ll->baudRate | CS8 | CLOCAL | CREAD;
	ll->newtio.c_iflag = IGNPAR;
	ll->newtio.c_oflag = 0;
	ll->newtio.c_lflag = 0;
	ll->newtio.c_cc[VTIME] = 10*ll->timeout;
	ll->newtio.c_cc[VMIN] = 0;

	tcflush(ll->fd, TCIOFLUSH);

	if (tcsetattr(ll->fd, TCSANOW, &ll->newtio) != 0)
		return -1;

	printf("New termios structure set.\n");

	int try = 0, connected = 0;
	switch (mode) {
	case SEND: {
		while (!connected) {
			if (try == 0 || alarmWentOff) {
				alarmWentOff = 0;
				if (try >= ll->numTries) {
					stopAlarm();
					printf("ERROR: Maximum number of retries reached. Aborting\n");
					return -1;
				}

				sendSUMessage(SET);
				if (++try == 1)
					setAlarm();
			}

			int type;
			unsigned char* receivedMessage = readMessage(&type, NULL);
			if (messageIs(receivedMessage, type, UA)) {
				connected = 1;
				printf("Estabilished connection\n");
			}
		}
		stopAlarm();
		break;
	}
	case RECEIVE: {
		while (!connected) {
			int type;
			unsigned char* receivedMessage = readMessage(&type, NULL);
			if (messageIs(receivedMessage, type, SET)) {
				sendSUMessage(UA);
				connected = 1;
				printf("Estabilished connection\n");
			}
		}
		break;
	}
	default:
		break;
	}

	return ll->fd;
}

int llwrite(const unsigned char* buf, unsigned int bufSize) {
	int try = 0, transferring = 1;
	while (transferring) {
		if (try == 0 || alarmWentOff) {
			alarmWentOff = 0;
			if (try >= ll->numTries) {
				stopAlarm();
				printf("ERROR: Maximum number of retries reached. Aborting\n");
				return -1;
			}

			sendIMessage(buf, bufSize);
			if (++try == 1)
				setAlarm();
		}

		int type;
		unsigned char* receivedMessage = readMessage(&type, NULL);

		if (messageIs(receivedMessage, type, RR)) {
			int nr = getMessageSequenceNumber(receivedMessage, type);
			if (ll->ns != nr)
				ll->ns = nr;

			stopAlarm();
			transferring = 0;
		}
		else if (messageIs(receivedMessage, type, REJ)) {
			stopAlarm();
			try = 0;
		}
	}

	stopAlarm();
	return 1;
}

int llread(unsigned char** message) {
	unsigned char* msg = NULL;
	int msgSize = 0;
	int done = 0;
	while (!done) {
		int type, hasBCC2Error, msgNs;
		msg = readMessage(&type, &hasBCC2Error);
		switch (type) {
		case INV_MESSAGE:
			printf("INVALID message received.\n");
			if (hasBCC2Error)
				sendSUMessage(REJ);
			break;
		case SU_MESSAGE:
			if (messageIs(msg, type, DISC))
				done = 1;
			break;
		case I_MESSAGE:
			msgNs = getMessageSequenceNumber(msg, type);
			msgSize = getIMessageSize(msg);
			if (ll->ns == msgNs) {
				*message = malloc(msgSize);
				memcpy(*message, &msg[4], msgSize);
				free(msg);

				ll->ns = !msgNs;
				sendSUMessage(RR);
				done = 1;
			} else {
				printf("Unexpected sequence number: ignoring content but confirming with RR.\n");
				sendSUMessage(RR);
			}
			break;
		}
	}

	stopAlarm();
	return msgSize;
}

int llclose(int mode) {
	printf("Ending connection\n");
	int try = 0, disconnected = 0;
	int type;
	unsigned char* receivedMessage;

	switch (mode) {
	case SEND: {
		while (!disconnected) {
			if (try == 0 || alarmWentOff) {
				alarmWentOff = 0;
				if (try >= ll->numTries) {
					stopAlarm();
					printf("ERROR: Maximum number of retries reached. Aborting\n");
					return -1;
				}

				sendSUMessage(DISC);
				if (++try == 1)
					setAlarm();
			}

			receivedMessage = readMessage(&type, NULL);
			if (messageIs(receivedMessage, type, DISC))
				disconnected = 1;
		}

		stopAlarm();
		sendSUMessage(UA);
		sleep(1);
		break;
	}
	case RECEIVE: {
		while (!disconnected) {
			receivedMessage = readMessage(&type, NULL);
			if (messageIs(receivedMessage, type, DISC))
				disconnected = 1;

			}

		int uaReceived = 0;
		while (!uaReceived) {
			if (try == 0 || alarmWentOff) {
				alarmWentOff = 0;

				if (try >= ll->numTries) {
					stopAlarm();
					printf("ERROR: Maximum number of retries reached. Aborting.\n");
					return -1;
				}

				sendSUMessage(DISC);
				if (++try == 1)
					setAlarm();
			}

			receivedMessage = readMessage(&type, NULL);
			if (messageIs(receivedMessage, type, UA))
				uaReceived = 1;
		}

		stopAlarm();
		printf("Ended connection\n");
	}
	default:
		break;
	}

	if (tcsetattr(ll->fd, TCSANOW, &ll->oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	close(ll->fd);

	switch (mode) {
		case SEND:
		printf("Connection ended\n");
		printf("SENT Messages : %d\n", ll->sentMessages);
		printf("Timeouts : %d\n", ll->timeouts);
		printf("RECEIVED RR Commands : %d\n", ll->numReceivedRR);
		printf("RECEIVED REJ Commands : %d\n", ll->numReceivedREJ);
		break;
		case RECEIVE:
		printf("RECEIVED Messages : %d\n", ll->receivedMessages);
		printf("SENT RR Commands : %d\n", ll->numSentRR);
		printf("SENT REJ Commands : %d\n", ll->numSentREJ);
		break;
	}

	return 1;
}

void sendSUMessage(int command) {
	int ctrlField;
	switch (command) {
	case SET:
		ctrlField = C_SET;
		break;
	case UA:
		ctrlField= C_UA;
		break;
	case RR:
		ctrlField= C_RR;
		break;
	case REJ:
		ctrlField= C_REJ;
		break;
	case DISC:
		ctrlField= C_DISC;
		break;
	default:
		return; //Unrecognised command
	}

	//Creating the package
	unsigned char* messageBuf = (unsigned char*)malloc(SU_MESSAGE_SIZE);

	messageBuf[0] = FLAG;
	messageBuf[1] = A;
	messageBuf[2] = ctrlField;
	if (ctrlField == C_REJ || ctrlField == C_RR)
		messageBuf[2] |= (ll->ns << 7);
	messageBuf[3] = messageBuf[1] ^ messageBuf[2];
	messageBuf[4] = FLAG;

	//Actually sending
	if (write(ll->fd, messageBuf, SU_MESSAGE_SIZE) != SU_MESSAGE_SIZE) {
		printf("ERROR: Could not write command with C 0x%x.\n", ctrlField);
		return;
	}

	free(messageBuf);

	if (command == REJ)
		ll->numSentREJ++;
	else if (command == RR)
		ll->numSentRR++;
}

void sendIMessage(const unsigned char* message, unsigned int messageSize) {
	//Creating the package
	unsigned char* msg = malloc(I_MESSAGE_BASE_SIZE + messageSize);

	unsigned char C = ll->ns << 6;
	unsigned char BCC1 = A ^ C;
	unsigned char BCC2 = getBCC2(message, messageSize);

	msg[0] = FLAG;
	msg[1] = A;
	msg[2] = C;
	msg[3] = BCC1;
	memcpy(&msg[4], message, messageSize);
	msg[4 + messageSize] = BCC2;
	msg[5 + messageSize] = FLAG;

	messageSize += I_MESSAGE_BASE_SIZE;
	messageSize = stuff(&msg, messageSize);

	//Actually sending
	unsigned int numWrittenBytes = write(ll->fd, msg, messageSize);
	if (numWrittenBytes != messageSize){
		printf("Could not write data packet correctly\n");
		return;
	}

	free(msg);
	ll->sentMessages++;
}

unsigned char* readMessage(int* type, int* hasBCC2Error) {
	*type = INV_MESSAGE;
	machineState state = START;
	unsigned int size = 0;
	unsigned char* message = malloc(ll->messageDataSize);
	volatile int done = 0;

	while (!done) {
		unsigned char c;
		if (state != STOP) {
			int numReadBytes = read(ll->fd, &c, 1);
			if (numReadBytes == 0) {
				printf("ERROR: Nothing received.\n");
				free(message);
				*type = INV_MESSAGE;
				if(hasBCC2Error != NULL)
					hasBCC2Error = 0;

				return NULL;
			}
		}

		switch (state) {
		case START:
			if (c == FLAG) {
				message[size++] = c;
				state = FLAG_RCV;
			}
			break;
		case FLAG_RCV:
			if (c == A) {
				message[size++] = c;
				state = A_RCV;
			} else if (c != FLAG) {
				size = 0;
				state = START;
			}
			break;
		case A_RCV:
			if (c != FLAG) {
				message[size++] = c;
				state = C_RCV;
			} else if (c == FLAG) {
				size = 1;
				state = FLAG_RCV;
			} else {
				size = 0;
				state = START;
			}
			break;
		case C_RCV:
			if (c == (message[1] ^ message[2])) {
				message[size++] = c;
				state = BCC_OK;
			} else if (c == FLAG) {
				size = 1;
				state = FLAG_RCV;
			} else {
				size = 0;
				state = START;
			}
			break;
		case BCC_OK:
			if (c == FLAG) {
				if(*type == INV_MESSAGE)
					*type = SU_MESSAGE;
				message[size++] = c;
				state = STOP;
			} else if (c != FLAG) {
				if(*type == INV_MESSAGE)
					*type = I_MESSAGE;

				// If extra memory is needed
				if (size % ll->messageDataSize == 0)
					message = (unsigned char*) realloc(message, size + ll->messageDataSize);

				message[size++] = c;
			}
			break;
		case STOP:
			message[size] = 0;
			done = 1;
			break;
		default:
			break;
		}
	}

	size = destuff(&message, size);
	unsigned char A = message[1];
	unsigned char C = message[2];
	unsigned char BCC1 = message[3];

	// Force errors on control packet (based on random value)
	int random = rand() % 100;
	if(random < RANDOM_ER_PROB) {
		printf("Forced ERROR on BCC1.\n");
		BCC1 = (A ^ C) + 1111;
	}

	if (BCC1 != (A ^ C)) {
		printf("ERROR: invalid BCC1.\n");
		free(message);
		*type = INV_MESSAGE;
		if(hasBCC2Error != NULL)
			*hasBCC2Error = 0;

		return NULL;
	}

	if (*type == SU_MESSAGE) {
		int controlField = message[2];

		if ((controlField & 0x0F) == C_REJ) {
			printf("Received REJ command.\n");
			ll->numReceivedREJ++;
		}
		else if ((controlField & 0x0F) == C_RR) {
			printf("Received RR command.\n");
			ll->numReceivedRR++;
		}
	}
	else if (*type == I_MESSAGE) {
		unsigned char calcBCC2 = getBCC2(&message[4], size - I_MESSAGE_BASE_SIZE);
		unsigned char BCC2 = message[4 + size - I_MESSAGE_BASE_SIZE];

		// Force errors on data packet (based on random value)
		int random = rand() % 100;
		if(random < RANDOM_ER_PROB) {
			printf("Forced ERROR on BCC2.\n");
			calcBCC2 = BCC2 + 1111;
		}

		if (calcBCC2 != BCC2) {
			printf("ERROR: Invalid BCC2: 0x%02x != 0x%02x.\n", calcBCC2, BCC2);
			free(message);
			*type = INV_MESSAGE;
			if(hasBCC2Error != NULL)
				*hasBCC2Error = 1;

			return NULL;
		}

		ll->receivedMessages++;
	}

	return message;
}

int messageIs(unsigned char* msg, int msgType, int command) {
	if(msgType != SU_MESSAGE) return 0;
	switch(command){
	case SET:
		return msg[2] == C_SET;
	case UA:
		return msg[2] == C_UA;
	case RR:
		return (msg[2] & 0x0F) == C_RR;
	case REJ:
		return (msg[2] & 0x0F) == C_REJ;
	case DISC:
		return msg[2] == C_DISC;
	default:
		return 0;
	}
}

unsigned int stuff(unsigned char** buf, unsigned int bufSize) {
	unsigned int newBufSize = bufSize;

	//Calculating new size by checking how many octates need to be escaped
	int i;
	for (i = 1; i < bufSize - 1; ++i){
		if ((*buf)[i] == FLAG || (*buf)[i] == ESCAPE)
			newBufSize++;
	}

	unsigned char* newBuf = (unsigned char*)malloc(newBufSize);
	int escapedBytesOffset = 0;
	//Flag on each end
	newBuf[0] = FLAG;
	newBuf[newBufSize - 1] = FLAG;

	//Copy the rest with respective byte stuffing when necessary
	for(i = 1; i < bufSize - 1; ++i){
		if((*buf)[i] == FLAG || (*buf)[i] == ESCAPE){
			newBuf[i + escapedBytesOffset] = ESCAPE;
			newBuf[i + escapedBytesOffset + 1] = (*buf)[i] ^ 0x20;
			++escapedBytesOffset;
		}
		else
			newBuf[i + escapedBytesOffset] = (*buf)[i];
	}

	free(*buf);
	*buf = newBuf;
	return newBufSize;
}

unsigned int destuff(unsigned char** buf, unsigned int bufSize) {
	unsigned int newBufSize = bufSize;
	int i;
	for(i = 1; i < bufSize - 1; ++i) {
		if((*buf)[i] == ESCAPE)
			--newBufSize;
	}

	unsigned char* newBuf = (unsigned char*)malloc(newBufSize);
	int escapedBytesOffset = 0;

	//Flag on each end
	newBuf[0] = FLAG;
	newBuf[newBufSize - 1] = FLAG;
	//Copy the rest with respective byte stuffing when necessary
	for(i = 1; i < bufSize - 1; ++i){
		if((*buf)[i] == ESCAPE){
			newBuf[i - escapedBytesOffset] = (*buf)[i+1] ^ 0x20;
			++i;
			++escapedBytesOffset;
		}
		else
			newBuf[i - escapedBytesOffset] = (*buf)[i];
	}

	free(*buf);
	*buf = newBuf;
	return newBufSize;
}

unsigned char getBCC2(const unsigned char* buf, unsigned int size) {
	unsigned char BCC2 = buf[0];

	unsigned int i;
	for (i = 1; i < size; ++i)
		BCC2 ^= buf[i];

	return BCC2;
}

int getMessageSequenceNumber(unsigned char* msg, int type){
	if(type == SU_MESSAGE) return (msg[2] >> 7) & 0x01;
	if(type == I_MESSAGE) return (msg[2] >> 6) & 0x01;
	return -1;
}

int getIMessageSize(unsigned char* message){
	if(message[4] == 1) //Data packet
		//4 bytes (C, N, L1, L2) + data size calculated by 256*L2+L1
		return 4 + (message[6] << 8) + message[7];
	//Else, a control package
	//5 bytes (C, T1, L1, T2, L2) + size of each file parameter given by L1 and L2
	return 5 + message[6] + message[7+message[6]+1];
}
