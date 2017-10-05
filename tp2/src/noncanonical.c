/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

volatile int STOP=FALSE;

typedef enum { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP_SET } setReceivedState;

int set_serialPort(int argc, char** argv, struct termios* oldtio, struct termios* newtio);
int reset_serialPort(int fd, struct termios* oldtio);
int setConnection(int fd);
int stateMachine(char test, setReceivedState* CURRENT_STATE);
int read_write_message(int fd);
void atende();


int main(int argc, char** argv) {

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

	//Ready do read a message and send back
	if((error = read_write_message(fd)) == -1)
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

	if ( tcgetattr(fd,oldtio) == -1) { /* save CURRENT_STATE port settings */
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
	  leitura do(s) proximo(s) caracter(es)
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
	unsigned char UA[5] = {FLAG, A, C_UA, (A^C_UA), FLAG};

	setReceivedState CURRENT_STATE = START;
	char buf[255];
	int res = 0;

	int connected = 0;
	

	//Receive SET packet and check if it's correct
	while(connected == 0){
		res = read(fd, buf, 1);
		buf[res]=0;

		connected = stateMachine(buf[0], &CURRENT_STATE);
			
	}

	write(fd, UA, 5); //Sent UA packet

	return 0;
}


int stateMachine(char test, setReceivedState* CURRENT_STATE){
	switch(*CURRENT_STATE){
	case START:
		if(test == FLAG)
			*CURRENT_STATE = FLAG_RCV;
		break;
	case FLAG_RCV:
		if(test == A)
			*CURRENT_STATE = A_RCV;
		else if(test != FLAG)
			*CURRENT_STATE = START;
		break;
	case A_RCV:
		if(test == C_SET)
			*CURRENT_STATE = C_RCV;
		else
			if(test != FLAG)
				*CURRENT_STATE = START;
			else
				*CURRENT_STATE = FLAG_RCV;
		break;
	case C_RCV:
		if(test == (A^C_SET))
			*CURRENT_STATE = BCC_OK;
		else
			if(test != FLAG)
				*CURRENT_STATE = START;
			else
				*CURRENT_STATE = FLAG_RCV;
		break;
	case BCC_OK:
		if(test == FLAG){
			*CURRENT_STATE = STOP_SET;
			printf("SET was sucessfuly received!\n");
			return 1;
}
		else
			*CURRENT_STATE = START;
		break;
	case STOP_SET:
		return 1;
		break;
	default:
		break;
	}
	
	return 0;
}

int read_write_message(int fd){

	int res=0;
	char buf[255];
	char result[255];

	STOP=FALSE;

	while (STOP==FALSE) {
		res = read(fd,buf,255);
		buf[res] = 0;               
		if (buf[res-1]=='\0') STOP=TRUE;
		strcat(result,buf);
	}

	printf("Message: %s\n", result);

	tcflush(fd, TCIFLUSH);

	res = write(fd,result,strlen(result)+1);
	printf("%d bytes written\n", res);

	return 0;
}
