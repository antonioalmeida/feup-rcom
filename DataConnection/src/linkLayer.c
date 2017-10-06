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
		case START:
			if(input == FLAG)
				*CURRENT_STATE = FLAG_RCV;
			break;
		case FLAG_RCV:
			if(input == A)
				*CURRENT_STATE = A_RCV;
			else if(input != FLAG)
				*CURRENT_STATE = START;
			break;
		case A_RCV:
			if(input == C_UA)
				*CURRENT_STATE = C_RCV;
			else
				if(input != FLAG)
					*CURRENT_STATE = START;
			else
				*CURRENT_STATE = FLAG_RCV;
			break;
		case C_RCV:
			if(input == (A^C_UA))
				*CURRENT_STATE = BCC;
			else
				if(input != FLAG)
					*CURRENT_STATE = START;
				else
					*CURRENT_STATE = FLAG_RCV;
			break;
		case BCC:
			if(input == FLAG){
				*CURRENT_STATE = STOP_UA;
				printf("UA was sucessfuly received!\n");
				flag = 1;
				return TRUE;
			}
			else
				*CURRENT_STATE = START;
			break;
			case STOP_UA:
				flag = 1;
				return TRUE;
				break;
			default:
				break;
	}

	return 0;
}