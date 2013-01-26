/*
*	This file contains all routines that handles the messages
*
*	SEND MESSAGE
*	RECEIVE MESSAGE
*
*/
#include			"global.h"
#include			"syscalls.h"
#include			"protos.h"
#include			"string.h"

#include			"userdefs.h"

/*
* Send Message
*	
*	This routine places a message string on to the current
*	processes Outbox. If the destination process has a ready
*	to receive message state, then the message is placed on to
*	the destination's Inbox. If the destination is not Ready to
*	receive, then its message state is set to Ready to receive.
*
*	A success or failure is passed to the OS if the message cannot
*	be sent, and an error occured.
*
*/
 void send_Message ( INT32 dest_ID, char *message, INT32 msg_Len, INT32 *error ){

 	//Check that dest_ID exists
 	INT32 check;
 	CALL( check = check_pid_ID(dest_ID) );
 	if (dest_ID == -1) check = 1;
 	//Failure
 	if (check == -1){
 		debugPrint("ERROR IN SEND MESSAGE: THAT DEST ID DOES NOT EXIST!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_Len > MAX_MSG){
 		debugPrint("ERROR IN SEND MESSAGE: THE MSG SIZE IS TOO LARGE!");
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
		debugPrint("ERROR IN SEND MESSAGE: OUTBOX MAX EXCEEDED");
		(*error) = ERR_BAD_PARAM;
		return;
	}
	else (*error) = ERR_SUCCESS;

	//STATE PRINTOUT FOR SEND
	if (SENDpo && DEBUGFLAG){
		SP_setup( SP_TARGET_MODE, current_PCB->p_id );
		SP_setup( SP_SEND_MODE, current_PCB->p_id);
		SP_print_header();
       	SP_print_line();
       	printf("\nSENDING MESSAGE: ");
       	printMessages(MSG);
	}

	current_PCB->msg_state = SEND_MSG;

	//Send if Dest is Ready to Receive, and set Dest to Ready otherwise
	CALL( send_if_dest_Receive(MSG, dest_ID) );
	CALL( target_to_Receive(dest_ID) );
}
//Helper function used to send message if the Dest
//message status is Ready to receive
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
//Helper function to sent the destination message state
//to ready to Receive
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
*
*	This function checks the outbox of processes for a 
*	message that matches its process ID. It places the
*	received message on to its Inbox, and checks the Inbox
*	for the message.
*
*	These steps were split out so that the message could also
*	be checked in os_switch_context_complete.
*
*	If receive_Message is called and it's message state is not
*	Ready to Receive, then it suspends itself until a message
*	is sent to it. In this situation, when the process resumes,
*	it can check its inbox in os_switch_context_complete, and 
*	pass the message to the OS.
*
*/
void receive_Message ( INT32 src_ID, char *message, 
	INT32 msg_rcvLen, INT32 *msg_sndLen, INT32 *sender_ID,  INT32 *error){

	MSG_t * msgRecv = NULL;

	//Check that the src_ID exists
	INT32 check;
	CALL( check = check_pid_ID(src_ID) );
	if (src_ID == -1) check = 1;
	if (check == -1){
 		debugPrint("ERROR IN RECV MESSAGE: THAT SRC ID DOES NOT EXIST!");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_rcvLen > MAX_MSG){
 		debugPrint("ERROR IN RECV MESSAGE: THE MSG SIZE IS TOO LARGE!");
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

 	CALL( add_to_Inbox(current_PCB, msgRecv) );
 	CALL( get_msg_Inbox(message, msg_sndLen, sender_ID) );

 	//Ensure message being sent is <= receive length
 	if (*msg_sndLen > msg_rcvLen){
 		debugPrint("ERROR IN RECV: MESSAGE SEND LENGTH > MESSAGE RECV LENGTH");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	*error = ERR_SUCCESS;
 	current_PCB->msg_state = READY_MSG;
}
//Get First Inbox Message, set pointer to next message
//Pass result to the Operating System to be checked
// ALSO USED IN OS_SWITCH_CONTEXT_COMPLETE
void get_msg_Inbox ( char *message, INT32 *msg_sndLen, INT32 *sender_ID){
	if (current_PCB->Inbox == NULL) return;

	MSG_t *ptrMsg = current_PCB->Inbox;

	*msg_sndLen = ptrMsg->Length;
 	strcpy(message, ptrMsg->message);
 	*sender_ID = ptrMsg->src_ID;

 	current_PCB->Inbox = ptrMsg->next;

 	if (SENDpo && DEBUGFLAG){
		SP_setup( SP_TARGET_MODE,current_PCB->p_id );
		SP_setup( SP_RECEIVE_MODE, current_PCB->p_id);
		SP_print_header();
       	SP_print_line();
       	printf("\nRECEIVED MESSAGE: ");
       	printMessages(ptrMsg);
	}
}
//Find a message on a PCB's outbox and return it
//Return NULL is no message exists
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

