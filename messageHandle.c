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
 	//Failure
 	if (check == -1){
 		printf("SEND MESSAGE: THAT DEST ID DOES NOT EXIST!\n");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_Len > MAX_MSG){
 		printf("SEND MESSAGE: THE MSG SIZE IS TOO LARGE!\n");
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
	INT32 check2;
	CALL( check2 = add_to_msgQueue(MSG) );
	if (check2 == -1){
		printf("SEND MESSAGE: OUTBOX MAX EXCEEDED\n");
		(*error) = ERR_BAD_PARAM;
		return;
	}
	else (*error) = ERR_SUCCESS;
}
/*
* Receive Message
*/
void receive_Message ( INT32 src_ID, char *message, 
	INT32 msg_rcvLen, INT32 *msg_sndLen, INT32 *sender_ID, INT32 *error){

	//Check that the src_ID exists
	INT32 check;
	CALL( check = check_pid_ID(src_ID) );
	if (check == -1){
 		printf("RECV MESSAGE: THAT SRC ID DOES NOT EXIST!\n");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
 	//Check the message size
 	if (msg_rcvLen > MAX_MSG){
 		printf("RECV MESSAGE: THE MSG SIZE IS TOO LARGE!\n");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}

 	if (*msg_sndLen < msg_rcvLen){
 		printf("RECV MESSAGE: BUFFER SIZE SENT TOO SMALL\n");
 		(*error) = ERR_BAD_PARAM;
 		return;
 	}
}
