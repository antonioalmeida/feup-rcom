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

typedef enum { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP_SET } setReadState;

int main(int argc, char** argv) {
	unsigned char SET[5] = {FLAG, A, C_SET, (A^C_SET), FLAG};
	unsigned char UA[5] = {FLAG, A, C_UA, (A^C_UA), FLAG};

	int fd,c, res;
	struct termios oldtio,newtio;

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

	if ( tcgetattr(fd,&oldtio) == -1) { /* save CURRENT_STATE port settings */
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

	char result[255];
	char currentByte;
	int i = 0;

	int receivedSET = 0;
	setReadState CURRENT_STATE = START;

	char message[255];

	while (STOP == FALSE) {       /* loop for input */
		res = read(fd, result, 1);   /* returns after 1 char have been input */
		result[res]=0;

		if(receivedSET == 0) {
			printf("Current byte : %s\n", result);
			//  STOP=TRUE;
			switch(CURRENT_STATE) {
			case START:
				if(result[0] == FLAG)
					CURRENT_STATE = FLAG_RCV;
				printf("Start.");
				break;

			case FLAG_RCV:
				printf("Flag received.");
				if(result[0] == A)
					CURRENT_STATE = A_RCV;
				else if(result[0] != FLAG)
					CURRENT_STATE = START;
				break;

			case A_RCV:
				if(result[0] == C_SET)
					CURRENT_STATE = C_RCV;
				else if(result[0] != FLAG)
					CURRENT_STATE = START;
				else
					CURRENT_STATE = FLAG_RCV;
				break;

			case C_RCV:
				if(result[0] == A^C_SET)
					CURRENT_STATE = BCC_OK;
				else if(result[0] != FLAG)
					CURRENT_STATE = START;
				else
					CURRENT_STATE = FLAG_RCV;
				break;

			case BCC_OK:
				if(result[0] == FLAG)
					CURRENT_STATE = STOP_SET;
				else
					CURRENT_STATE = START;

			case STOP_SET:
				printf("SET was successfuly received.\n");
				receivedSET = 1;
				write(fd, UA, 5);
				STOP = TRUE;

				break;
			default:
				break;
			}
		}
		else { // SET RECEIVED
			result[i++] = currentByte;
			if (currentByte == '\0')
				STOP=TRUE;
		}

		printf(":%s\n", result);
	}

	//ler o bite lixo?
	res = read(fd, result, 1);   /* returns after 1 char have been input */
	result[res]=0;


	STOP=FALSE;
	char temp[255];
	while (STOP==FALSE) {       /* loop for input */
		res = read(fd,temp,255);   /* returns after 5 chars have been input */
		temp[res] = 0;               /* so we can printf... */
		printf(":%s:%d\n", temp, res);
		if (temp[res-1]=='\0') STOP=TRUE;
		strcat(result,temp);
	}

	printf("Message: %s.\n", result);
	tcflush(fd, TCIFLUSH);

	res = write(fd,result,strlen(result)+1);
	printf("%d bytes written\n", res);

	/*
    printf("Message: %s.\n", result);
    tcflush(fd, TCIFLUSH);

    res = write(fd,result,strlen(result)+1);
    printf("%d bytes written\n", res);
	 */
	/*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
	 */

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	return 0;
}
