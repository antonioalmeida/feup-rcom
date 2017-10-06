/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "linkLayer.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1
#define MAX_TRIES 3

volatile int STOP = FALSE;

#define TRANSMITTER 0;
#define RECEIVER 1;
int STATUS = -1; // Transmitter or receiver

int timer=1, flag=1;

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC, STOP_UA} uaReceivedState;
typedef enum { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP_SET } setReceivedState;

int checkArgs(int argc, char** argv);
int initSerialPort(struct termios* oldtio, struct termios* newtio);
int resetSerialPort(int fd, struct termios* oldtio);
int setConnection(int fd);
int testConnectionTimeout(int fd);
int receiveUAStateMachine(char test, uaReceivedState* current);

int writeMessage(int fd);
int readMessage(int fd);
void atende();

int main(int argc, char** argv) {
	struct termios oldtio,newtio;
	int error = 0;

	STATUS = checkArgs(argc, argv);

	if(STATUS < 0) {
		printf("STATUS definition error.");
		exit(STATUS);
	}

	//Initialize the serial port with proper configurations
	int fd = initSerialPort(&oldtio,&newtio);

	if(fd != 1 && fd != -1 )
		printf("New termios structure set\n");
	else
		exit(fd);

/*
	//Estabilishing connection
	testConnectionTimeout(fd);

	//Ready do send a message
	if((error = writeMessage(fd)) == -1)
		exit(error);

	//Ready to receive a message back
	if((error = readMessage(fd)) == -1)
		exit(error);


	if(resetSerialPort(fd,&oldtio)==-1)
		exit(-1);
	*/

	close(fd);
	return 0;
}

int checkArgs(int argc, char** argv) {

	if ( (argc < 3) ||
		((strcmp("write", argv[1]) != 0) && 
		(strcmp("read", argv[1]) != 0)
		(strcmp("/dev/ttyS0", argv[2]) != 0) &&
		(strcmp("/dev/ttyS1", argv[2])!=0) )) {
			printf("Usage:\tnserial SerialPort\n\tex: nserial <read/write> /dev/ttyS0\n");
			return -1;
		}

	if((strcmp("write", argv[1]) == 0))
		return TRANSMITTER;
	else if((strcmp("read", argv[1]) == 0))
		return RECEIVER;
		
	return -1;
}

int initSerialPort(struct termios* oldtio, struct termios* newtio){
	int fd;

	/*
	  Open serial port device for reading and writing and not as controlling tty
	  because we don't want to get killed if linenoise sends CTRL-C.
	 */

	if ((fd = open(argv[1], O_RDWR | O_NOCTTY )) <0) {
		perror(argv[1]);
		return -1;
	}

	if ( tcgetattr(fd,oldtio) == -1) { /* save current port settings */
	perror("tcgetattr");
	return -1;
}

bzero(newtio, sizeof(*newtio));
(*newtio).c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
(*newtio).c_iflag = IGNPAR;
(*newtio).c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
(*newtio).c_lflag = 0;

	(*newtio).c_cc[VTIME]    = 0;   /* inter-character timer unused */
	(*newtio).c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

	/*
	  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	  leitura do(s) proxiimo(s) caracter(es)
	 */

tcflush(fd, TCIOFLUSH);

if ( tcsetattr(fd,TCSANOW,newtio) == -1) {
	perror("tcsetattr");
	return -1;
}
return fd;
}

int resetSerialPort(int fd,struct termios* oldtio){
	if (tcsetattr(fd,TCSANOW,oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}
	return 0;
}

int setConnection(int fd) {
	unsigned char SET[5] = {FLAG, A, C_SET, (A^C_SET), FLAG};

	uaReceivedState current = START;
	write(fd, SET, 5); //SET packet sent

	int connected = 0;
	char buf[255];
	int res = 0;

	//Receive UA packet and check if it's correct
	while (connected == FALSE) {
		res = read(fd,buf,1);
		buf[res]=0;

		connected = receiveUAStateMachine(buf[0], &current);
	}

	return 0;

}

int testConnectionTimeout(int fd){
	unsigned char SET[5] = {FLAG, A, C_SET, (A^C_SET), FLAG};

	(void) signal(SIGALRM, atende);
	uaReceivedState current = START;
	write(fd, SET, 5); //SET packet sent

	int connected = 0;
	char buf[255];
	int res = 0;

	int j;
	for(j=0; j <= MAX_TRIES; j++){
		if(connected == 1)
			break;

	//Receive UA packet and check if it's correct
		while (timer < 4 && connected == FALSE) {

			if(flag){
      		alarm(3);  // activa alarme de 3s
      		flag=0;
      	}

      	res = read(fd,buf,1);
      	buf[res]=0;

      	connected = receiveUAStateMachine(buf[0], &current);

      }
  }

  return 0;

}


int writeMessage(int fd){
	char buf[255];
	int res=0;

	if(gets(buf) == NULL) {
		perror("Error ocurred on getting the message!");
		return -1;
	}

	res = write(fd,buf,strlen(buf)+1);
	printf("%d bytes written\n", res);

	return 0;
}

int readMessage(int fd){
	int n = 0;
	int res=0;
	char result[255];

	STOP=FALSE;
	while (STOP==FALSE) {
		res = read(fd,result+n++,1);
		if (result[n]=='\0') STOP=TRUE;
	}

	printf("%s\n", result);
	printf("%d bytes read\n", n+1);

	return 0;
}

void atende(){
	flag=1;
	timer++;
}
