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
 	MSG->src_ID = current_PCB->p_id;
 	MSG->Length = msg_Len;
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
	CALL( target_to_Receive(dest_ID) );
	current_PCB->msg_state = SEND_MSG;
}
void target_to_Receive ( INT32 dest_ID ){
	ZCALL( lockReady() );
	PCB_t * ptrCheck = pidList;

	while(ptrCheck!=NULL){
		if(ptrCheck->p_id == dest_ID){
			ptrCheck->msg_state = RECEIVE_MSG;
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

 	if ( current_PCB->msg_state != RECEIVE_MSG ){
 		INT32 eRRor;
 		CALL( suspend_Process(-1, &eRRor) );
 	}

 	if (src_ID == -1){
 		CALL( msgRecv = get_Message(current_PCB->p_id) );
 	}
 	else{
 		CALL( msgRecv = get_Message(src_ID) );
 	}
 	if (msgRecv != NULL){ 		
 		*msg_sndLen = msgRecv->Length;
 		strcpy(message, msgRecv->message);
 		*sender_ID = msgRecv->src_ID;
 	}
 	//Ensure message being sent is >= recenve length
// 	printf("*****SIZE: %d\n\n", *msg_sndLen);
 	if (*msg_sndLen > msg_rcvLen){
 		debugPrint("IN RECV: MESSAGE SEND LENGTH < MESSAGE RECV LENGTH");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Search for message
}

MSG_t * check_Inbox ( INT32 src_ID ){
	ZCALL( lockReady() );
	MSG_t * msgCheck = current_PCB->Inbox;

	while (msgCheck != NULL){
		if ( msgCheck->src_ID == src_ID || src_ID == -1 ){
			ZCALL( unlockReady() );
			return msgCheck;
		}		
		msgCheck = msgCheck->next;
	}	
	ZCALL( unlockReady() );
	return NULL;
}

MSG_t * get_Message ( INT32 src_ID ){
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
			CALL( remove_first_MSG(ptrCheck->Outbox) );
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
void remove_first_MSG ( MSG_t * remove ){
	MSG_t * temp = remove->next;
	remove = temp;
	return;
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
