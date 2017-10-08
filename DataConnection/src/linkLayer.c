#include <stdio.h>
#include "linkLayer.h"

int flag = 1; //Not sure why we need this but ok

int receiveSETStateMachine(char input, setReceivedState* CURRENT_STATE) {
	switch(*CURRENT_STATE) {
	case SET_START:
		if(input == FLAG)
			*CURRENT_STATE = SET_FLAG_RCV;
		break;
	case SET_FLAG_RCV:
		if(input == A)
			*CURRENT_STATE = SET_A_RCV;
		else if(input != FLAG)
			*CURRENT_STATE = SET_START;
		break;
	case SET_A_RCV:
		if(input == C_SET)
			*CURRENT_STATE = SET_C_RCV;
		else
			if(input != FLAG)
				*CURRENT_STATE = SET_START;
			else
				*CURRENT_STATE = SET_FLAG_RCV;
		break;
	case SET_C_RCV:
		if(input == (A^C_SET))
			*CURRENT_STATE = SET_BCC_OK;
		else
			if(input != FLAG)
				*CURRENT_STATE = SET_START;
			else
				*CURRENT_STATE = SET_FLAG_RCV;
		break;
	case SET_BCC_OK:
		if(input == FLAG) {
			*CURRENT_STATE = SET_STOP;
			printf("SET was sucessfuly received!\n");
			return TRUE;
		}
		else
			*CURRENT_STATE = SET_START;
		break;
	case SET_STOP:
		return TRUE;
		break;
	default:
		break;
	}

	return FALSE;
}
 // TO DO: update these values
int receiveUAStateMachine(char input, uaReceivedState* CURRENT_STATE) {
	switch(*CURRENT_STATE) {
		case UA_START:
			if(input == FLAG)
				*CURRENT_STATE = UA_FLAG_RCV;
			break;
		case UA_FLAG_RCV:
			if(input == A)
				*CURRENT_STATE = UA_A_RCV;
			else if(input != FLAG)
				*CURRENT_STATE = UA_START;
			break;
		case UA_A_RCV:
			if(input == C_UA)
				*CURRENT_STATE = UA_C_RCV;
			else
				if(input != FLAG)
					*CURRENT_STATE = UA_START;
			else
				*CURRENT_STATE = UA_FLAG_RCV;
			break;
		case UA_C_RCV:
			if(input == (A^C_UA))
				*CURRENT_STATE = UA_BCC_OK;
			else
				if(input != FLAG)
					*CURRENT_STATE = UA_START;
				else
					*CURRENT_STATE = UA_FLAG_RCV;
			break;
		case UA_BCC_OK:
			if(input == FLAG){
				*CURRENT_STATE = UA_STOP;
				printf("UA was sucessfuly received!\n");
				flag = 1;
				return TRUE;
			}
			else
				*CURRENT_STATE = UA_START;
			break;
			case UA_STOP:
				flag = 1;
				return TRUE;
				break;
			default:
				break;
	}

	return 0;
}
