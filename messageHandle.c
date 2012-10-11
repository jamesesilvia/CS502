#include			"global.h"
#include			"syscalls.h"
#include			"protos.h"
#include			"string.h"

#include			"userdefs.h"

/*
* Send Message
*/
 void send_Message ( INT32 dest_ID, char *message, INT32 msg_Len, INT32 *error ){

 	//Check that dest_ID exists
 	INT32 check;
 	CALL( check = check_pid_ID(dest_ID) );
 	if (dest_ID == -1) check = 1;
 	//Failure
 	if (check == -1){
 		debugPrint("IN SEND MESSAGE: THAT DEST ID DOES NOT EXIST!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_Len > MAX_MSG){
 		debugPrint("IN SEND MESSAGE: THE MSG SIZE IS TOO LARGE!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}

 	//No errors, build message
 	MSG_t *MSG = (MSG_t *)(malloc(sizeof(MSG_t)));
 	MSG->dest_ID = dest_ID;
 	memset(MSG->message, 0, MAX_MSG+1);
	strcpy(MSG->message, message);
	MSG->next = NULL;

	//Add the message to the current PCBs Outbox
	CALL( check = add_to_msgQueue(MSG) );
	if (check == -1){
		debugPrint("IN SEND MESSAGE: OUTBOX MAX EXCEEDED");
		(*error) = ERR_BAD_PARAM;
		return;
	}	
	else (*error) = ERR_SUCCESS;
	current_PCB->msg_state = SEND_MSG;
}
/*
* Receive Message
*/
void receive_Message ( INT32 src_ID, char *message, 
	INT32 msg_rcvLen, INT32 *msg_sndLen, INT32 *sender_ID, INT32 *error){
	MSG_t * msgRecv = NULL;

	//Check that the src_ID exists
	INT32 check;
	CALL( check = check_pid_ID(src_ID) );
	if (src_ID == -1) check = 1;
	if (check == -1){
 		debugPrint("IN RECV MESSAGE: THAT SRC ID DOES NOT EXIST!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_rcvLen > MAX_MSG){
 		debugPrint("IN RECV MESSAGE: THE MSG SIZE IS TOO LARGE!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}

 	//Search for message
 	if (src_ID == -1) CALL( msgRecv = get_Message() );

 	//Need to discuss this check
 	//Is it supposed to be less than? greater than? equal to?
 	if (*msg_sndLen != msg_rcvLen){
 		debugPrint("IN MESSAGE SEND LENGTH != MESSAGE RECV LENGTH");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
}

MSG_t * get_Message ( void ){
	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	while (ptrCheck != NULL){
		MSG_t * msgCheck = ptrCheck->Outbox;
		while (msgCheck != NULL){
			if ( msgCheck->dest_ID == -1 || msgCheck->dest_ID == current_PCB->p_id ){
				ZCALL( unlockReady() );
				return msgCheck;
			}
			msgCheck = msgCheck->next;
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	return NULL;
}
