#include <termios.h> // For baudrate constants
#include <stdio.h>
#include "Utilities.h"

int getIntegerInRange(int lower, int upper) {
	int number;

	int valid = 0;
	while (!valid) {
		if (scanf("%d", &number) == 1 && lower <= number && number <= upper)
			valid = 1;
		else
			printf("Number must be in range [%d, %d]: ", lower, upper);

		clearStdIn();
	}

	return number;
}

int getBaudrate() {
	int number;
	do{
		number = getIntegerInRange(0, 115200);
		switch (number) {
		case 1200:
			return B1200;
		case 1800:
			return B1800;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		default:
			break;
		}
		printf("Invalid baudrate. Choose one from the shown list: ");
	}while(1);
}

void clearStdIn() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
}
