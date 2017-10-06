#define MAX_SIZE 255
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

struct linkLayer {
	char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate; /*Velocidade de transmissão*/
	unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
	unsigned int timeout; /*Valor do temporizador: 1 s*/
	unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
	char frame[MAX_SIZE]; /*Trama*/
};


typedef enum { UA_START, _UA_FLAG_RCV, UA_A_RCV, UA_C_RCV, UA_BCC_OK, UA_STOP } uaReceivedState;
typedef enum { SET_START, SET_FLAG_RCV, SET_A_RCV, SET_C_RCV, SET_BCC_OK, SET_STOP } setReceivedState;

int receiveSETStateMachine(char test, setReceivedState* CURRENT_STATE);
int receiveUAStateMachine(char test, uaReceivedState* current);

