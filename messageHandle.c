#include			"global.h"
#include			"syscalls.h"
#include			"protos.h"
#include			"string.h"

#include			"userdefs.h"

/*
* Send Message
*/
 void send_Message ( INT32 dest_ID, char *message, INT32 msg_Len, INT32 *error ){

 	/* Helper prints for Debugging
 	printf("\nSENDING MESSAGE TO: %d\n", dest_ID);
 	printf("WITH A MESSAGE OF: %s\n", message);
 	printf("OF LENGTH: %d\n\n", msg_Len);
	*/

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
 	MSG->src_ID = current_PCB->p_id;
 	MSG->Length = msg_Len;
 	memset(MSG->message, 0, MAX_MSG+1);
	strcpy(MSG->message, message);
	MSG->next = NULL;

	//Add the message to the current PCBs Outbox
	CALL( check = add_to_Outbox(MSG) );
	if (check == -1){
		debugPrint("IN SEND MESSAGE: OUTBOX MAX EXCEEDED");
		(*error) = ERR_BAD_PARAM;
		return;
	}	
	else (*error) = ERR_SUCCESS;

	current_PCB->msg_state = SEND_MSG;

	//Send if Dest is Ready to Receive, and set Dest to Ready otherwise
	CALL( send_if_dest_Receive(MSG, dest_ID) );
	CALL( target_to_Receive(dest_ID) );
}

void send_if_dest_Receive( MSG_t *tosend, INT32 dest_ID ){
	PCB_t *dest = pidList;
	while (dest!=NULL){
		if (dest->p_id == dest_ID){
			if (dest->msg_state == RECEIVE_MSG){
				add_to_Inbox(dest, tosend);
				return;
			}
		}
		dest = dest->next;
	}
}
void target_to_Receive ( INT32 dest_ID ){
	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	while(ptrCheck!=NULL){
		if(ptrCheck->p_id == dest_ID){
			ptrCheck->msg_state = RECEIVE_MSG;
			ptrCheck->p_state = READY_STATE;
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
}
/*
* Receive Message
*/
void receive_Message ( INT32 src_ID, char *message, 
	INT32 msg_rcvLen, INT32 *msg_sndLen, INT32 *sender_ID,  INT32 *error){

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
 	//If not ready to receive a message, suspend.
 	if ( current_PCB->msg_state != RECEIVE_MSG ){
 		INT32 eRRor;
 		current_PCB->msg_state = RECEIVE_MSG;
 		CALL( suspend_Process(-1, &eRRor) );
 	}

 	//Get any message with dest of currentPCBs ID 
 	if (src_ID == -1){
 		CALL( msgRecv = get_outboxMessage(current_PCB->p_id) );
 	}
 	//Else get message with source ID
 	else{
 		CALL( msgRecv = get_outboxMessage(src_ID) );
 	}

 	if (msgRecv == NULL) printf("SOMETHING WENT WAYYYY WRONGGGGG\n\n\n");

 	CALL( add_to_Inbox(current_PCB, msgRecv) );
 	CALL( get_msg_Inbox(message, msg_sndLen, sender_ID) );

 	//Ensure message being sent is <= receive length
 	if (*msg_sndLen > msg_rcvLen){
 		debugPrint("IN RECV: MESSAGE SEND LENGTH > MESSAGE RECV LENGTH");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	*error = ERR_SUCCESS;
 	current_PCB->msg_state = READY_MSG;
}
//Get First Inbox Message, set pointer to next message
void get_msg_Inbox ( char *message, INT32 *msg_sndLen, INT32 *sender_ID){
	if (current_PCB->Inbox == NULL) return;

	MSG_t *ptrMsg = current_PCB->Inbox;

	*msg_sndLen = ptrMsg->Length;
 	strcpy(message, ptrMsg->message);
 	*sender_ID = ptrMsg->src_ID;

 	current_PCB->Inbox = ptrMsg->next;

// 	if(current_PCB->Inbox == NULL) current_PCB->msg_state = READY_MSG;
}
//Find a message on a PCB's outbox and return it
MSG_t * get_outboxMessage ( INT32 src_ID ){
	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	while (ptrCheck != NULL){
		MSG_t * msgCheck = ptrCheck->Outbox;
		while (msgCheck != NULL){
			if ( (msgCheck->dest_ID == -1) || (msgCheck->dest_ID == src_ID) ){
				ptrCheck->Outbox = msgCheck->next;
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
/* NOT CURRENTLY USED
void exchange_Messages ( void ){
//	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	//Loop through main list for Outbox messages
	while (ptrCheck != NULL){
		MSG_t *msgCheck = ptrCheck->Outbox;
		while (msgCheck != NULL){
			if (msgCheck->dest_ID == -1){
				CALL( sendMSG_to_all(msgCheck) );
			}
			else{
				CALL( sendMSG_to_one(msgCheck) );
			}
			msgCheck = msgCheck->next;
		}
		ptrCheck = ptrCheck->next;
	}

}
void wakeUp_Messages ( void ){
	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	while (ptrCheck != NULL){
		if (ptrCheck->msg_state == RECEIVE_MSG){
			if (ptrCheck->p_state == SUSPENDED_STATE){
				ptrCheck->p_state = READY_STATE;
			}
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
}
void sendMSG_to_all ( MSG_t * tosend ){
	PCB_t * ptrCheck = pidList;

	while (ptrCheck != NULL){
		if (ptrCheck->p_id != tosend->src_ID ){
			MSG_t * Inbox = ptrCheck->Inbox;
			while(Inbox != NULL){
				Inbox = Inbox->next;
			}
			Inbox = tosend;
			ptrCheck->msg_state = RECEIVE_MSG;
		}
		ptrCheck = ptrCheck->next;
	}
}
void sendMSG_to_one ( MSG_t * tosend ){
	PCB_t * ptrCheck = pidList;

	while (ptrCheck != NULL){
		if (ptrCheck->p_id == tosend->dest_ID){			
			MSG_t * Inbox = ptrCheck->Inbox;
			while(Inbox != NULL){
				Inbox = Inbox->next;
			}
			Inbox = tosend;
			ptrCheck->msg_state = RECEIVE_MSG;
		}
		ptrCheck = ptrCheck->next;
	}
}
*/
