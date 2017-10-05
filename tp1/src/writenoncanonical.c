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

volatile int STOP=FALSE;

int set_serialPort(int argc, char** argv, struct termios* oldtio, struct termios* newtio);
int reset_serialPort(int fd, struct termios* oldtio);
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
