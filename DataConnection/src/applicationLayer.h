#pragma once

struct applicationLayer {
	int fileDescriptor;
/*Descritor correspondente à porta série*/
	int status;
/*TRANSMITTER | RECEIVER*/
};

int llopen(int porta, int status);

int llwrite(int fd, char * buffer, int length);

int llread(int fd, char * buffer);

int llclose(int fd);
