/* 	This file includes several functions that were used 
*	during debugging, particulary during the use of gdb.
*	In order to track queues and ensure proper management,
*	several functions were created to print out the full list.
*
*	This follows suit for any of the messages added to a PCB's
*	Inbox or Outbox.
*
*	These functions are included in many of the state print outs
*	as they do a good job of showing state changes and queue
*	management.
*
*	Also, the debugPrint function that depends on the DEBUGFLAG
*	in userdefs is built here.
*
*/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

//Print the Timer Queue
void printTimer ( void ){
	PCB_t * ptrCheck = timerList;
	printf("\n----------TIMER QUEUE---------\n");
	while (ptrCheck != NULL){		
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("WakeupTime: %d\t", ptrCheck->p_time);
		printf("Priority: %d\t", ptrCheck->p_priority);
		printf("State: %d\n", ptrCheck->p_state);
		printf("------------------------------\n");
		ptrCheck = ptrCheck->next;
	}
	printf("\n");
	return;
}
//Print the Ready Queue
void printReady ( void ){
	PCB_t * ptrCheck = pidList;
	printf("\n----------READY QUEUE---------\n");
	while (ptrCheck != NULL){		
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("WakeupTime: %d\t", ptrCheck->p_time);
		printf("Priority: %d\t", ptrCheck->p_priority);
		printf("State: %d\t", ptrCheck->p_state);
		printf("MSGState: %d\n", ptrCheck->msg_state);
		printf("------------------------------\n");
		ptrCheck = ptrCheck->next;
	}
	printf("\n");
	return;
}
//Print the Event Queue
void printEvent ( void ){
	EVENT_t * ptrCheck = eventList;
	while (ptrCheck != NULL){
		printf("------------------------------\n");
		printf("Device ID: %d\t", ptrCheck->device_ID);
		printf("Status: %d\t", ptrCheck->Status);
		ptrCheck = ptrCheck->next;	
	}
	printf("\n");
	return;
}
//Print Messages from either Inbox or Outbox
void printMessages ( MSG_t * Message ){
	while(Message != NULL){
		printf("MSG: %s\t", Message->message);
		printf("SRC ID: %d\t", Message->src_ID);
		printf("DEST ID: %d\n", Message->dest_ID);
		printf("------------------------------\n");
		Message = Message->next;
	}
	printf("\n");
	return;
}
// Helper function used to only print to screen
//	if the DEBUGFLAG is set in the userdefs.h file.
void debugPrint ( char * toprint ){
	if (DEBUGFLAG){
		printf("%s\n", toprint);
	}
}
