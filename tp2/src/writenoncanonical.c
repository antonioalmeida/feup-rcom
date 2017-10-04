/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1
#define MAX_TRIES 3

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

volatile int STOP=FALSE;
int timer=1, flag=1;
typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC, STOP_UA}uaReceivedState;

int set_serialPort(int argc, char** argv, struct termios* oldtio, struct termios* newtio);
int reset_serialPort(int fd, struct termios* oldtio);
int setConnection(int fd);
int stateMachine(char test, uaReceivedState* current);
int write_message(int fd);
int read_message(int fd);
void atende();

int main(int argc, char** argv)
{
	struct termios oldtio,newtio;
	int error = 0;
	//Set the serial port to right config
	int fd=set_serialPort(argc, argv,&oldtio,&newtio);
	if(fd != 1 || fd != -1 ){
		printf("New termios structure set\n");
	}else{
		exit(fd);
	}

	//Estabilishing connection
	setConnection(fd);

	//Ready do send a message
	if((error = write_message(fd)) == -1)
		exit(error);

	//Ready to receive a message back
	if((error = read_message(fd)) == -1)
		exit(error);


	if(reset_serialPort(fd,&oldtio)==-1)
		exit(-1);

	close(fd);
	return 0;
}

int set_serialPort(int argc, char** argv, struct termios* oldtio, struct termios* newtio){
	int fd;

	if ( (argc < 2) ||
			((strcmp("/dev/ttyS0", argv[1])!=0) &&
					(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
		return 1;
	}

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

int reset_serialPort(int fd,struct termios* oldtio){
	if (tcsetattr(fd,TCSANOW,oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}
	return 0;
}

int setConnection(int fd){
	unsigned char SET[5] = {FLAG, A, C_SET, (A^C_SET), FLAG};

	uaReceivedState current = START;
	write(fd, SET, 5); //SET packet sent

	int connected = 0;
	int uaReceived = 0;
	char buf[255];
	int res = 0;

	//Receive UA packet and check if it's correct
	while (connected == 0) {
		res = read(fd,buf,1);
		buf[res]=0;

		if(uaReceived == 0){
			uaReceived = stateMachine(buf[0], &current);
		} else {
			connected = 1;
		}
	}

	return 0;

}


int stateMachine(char test, uaReceivedState* current){
	switch(*current){
	case START:
		if(test == FLAG)
			*current = FLAG_RCV;
		break;
	case FLAG_RCV:
		if(test == A)
			*current = A_RCV;
		else if(test != FLAG)
			*current = START;
		break;
	case A_RCV:
		if(test == C_UA)
			*current = C_RCV;
		else
			if(test != FLAG)
				*current = START;
			else
				*current = FLAG_RCV;
		break;
	case C_RCV:
		if(test == (A^C_UA))
			*current = BCC;
		else
			if(test != FLAG)
				*current = START;
			else
				*current = FLAG_RCV;
		break;
	case BCC:
		if(test == FLAG)
			*current = STOP_UA;
		else
			*current = START;
		break;
	case STOP_UA:
		printf("UA was sucessfuly received!\n");
		return 1;
		break;
	default:
		break;
	}

	return 0;
}


int write_message(int fd){
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

int read_message(int fd){
	int n = 0;
	int res=0;
	char buf[255];

	while (STOP==FALSE) {
		if((res = read(fd,buf+n++,1)) == 1){
			if (buf[n]=='\0') STOP=TRUE;
		}
		else if (res == -1){
			printf("Error: can't read from serial port\n");
			return -1;
		}
	}

	printf("%s\n", buf);
	printf("%d bytes read\n", n+1);

	return 0;
}

void atende(){
	flag=1;
	timer++;
}
