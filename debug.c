#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

//Debug
void printTimer ( void ){
	PCB_t * ptrCheck = timerList;
	while (ptrCheck != NULL){
		printf("------------------------------\n");
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("Time: %d\t", ptrCheck->p_time);
		printf("Priority: %d\t", ptrCheck->p_priority);
		printf("State: %d\n", ptrCheck->p_state);
		ptrCheck = ptrCheck->next;
	}
	printf("\n");
	return;
}
void printReady ( void ){
	PCB_t * ptrCheck = pidList;
	while (ptrCheck != NULL){
		printf("------------------------------\n");
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("Time: %d\t", ptrCheck->p_time);
		printf("Priority: %d\t", ptrCheck->p_priority);
		printf("State: %d\t", ptrCheck->p_state);
		printf("MSGState: %d\n", ptrCheck->msg_state);
		ptrCheck = ptrCheck->next;
	}
	printf("\n");
	return;
}
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
void printMessages ( MSG_t * Message ){
	while(Message != NULL){
		printf("------------------------------\n");
		printf("MSG: %s\t", Message->message);
		printf("SRC ID: %d\t", Message->src_ID);
		printf("DEST ID: %d\n", Message->dest_ID);
		Message = Message->next;
	}
	printf("\n");
	return;
}
void debugPrint ( char * toprint ){
	if (DEBUGFLAG){
		printf("%s\n", toprint);
	}
}
