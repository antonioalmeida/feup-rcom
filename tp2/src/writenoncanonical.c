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
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

volatile int STOP=FALSE;
typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC, STOP_UA}uaReceivedState;

int main(int argc, char** argv)
{
	unsigned char SET[5] = {FLAG, A, C_SET, (A^C_SET), FLAG};
	unsigned char UA[5] = {FLAG, A, C_UA, (A^C_UA), FLAG};

	int fd,c, res;
	struct termios oldtio,newtio;
	char buf[255];
	int i, sum = 0, speed = 0;

	if ( (argc < 2) ||
			((strcmp("/dev/ttyS0", argv[1])!=0) &&
					(strcmp("/dev/ttyS1", argv[1])!=0) )) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}


	/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
	 */


	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

	/*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) pr�ximo(s) caracter(es)
	 */

	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");


	//Estabilishing connection
	uaReceivedState current = START;
	write(fd, SET, 5); //SET packet sent

	int connected = 0;
	int uaReceived = 0;


	while (connected==0) {

		res = read(fd,buf,1);
		buf[res]=0;

		if(uaReceived == 0){
			switch(current){
			case START:
				if(buf[0] == FLAG)
					current = FLAG_RCV;

				break;
			case FLAG_RCV:
				if(buf[0] == A){
					current = A_RCV;
				}
				else if(buf[0] != FLAG)
					current = START;
				break;
			case A_RCV:
				if(buf[0] == C_UA){
					current = C_RCV;
				}
				else
					if(buf[0] != FLAG)
						current = START;
					else
						current = FLAG_RCV;
				break;
			case C_RCV:
				if(buf[0] == A^C_UA){
					current = BCC;
				}
				else
					if(buf[0] != FLAG)
						current = START;
					else
						current = FLAG_RCV;
				break;
			case BCC:
				if(buf[0] == FLAG){
					current = STOP_UA;
				}
				else{
					current = START;
				}
			case STOP_UA:
				uaReceived = 1;
				connected = 1;
				printf("Recebeu UA!\n");
				break;

			default:
				break;
			}
		}
	}


	//Data packets dispatch

	if(gets(buf) == NULL) {
		perror("Error ocurred!");
		exit(-1);
	}

	res = write(fd,buf,strlen(buf)+1);
	printf("%d bytes written\n", res);

	/*
O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar
o indicado no gui�o
	 */

	int n = 0;
	while (STOP==FALSE) {
		if((res = read(fd,buf+n++,1)) == 1)
			if (buf[n]=='\0') STOP=TRUE;
	}

	printf("%s\n", buf);
	printf("%d bytes read\n", n+1);

	if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}
