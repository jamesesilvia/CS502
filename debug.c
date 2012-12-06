/*     This file includes several functions that were used 
*    during debugging, particulary during the use of gdb.
*    In order to track queues and ensure proper management,
*    several functions were created to print out the full list.
*
*    This follows suit for any of the messages added to a PCB's
*    Inbox or Outbox.
*
*    These functions are included in many of the state print outs
*    as they do a good job of showing state changes and queue
*    management.
*
*    Also, the debugPrint function that depends on the DEBUGFLAG
*    in userdefs is built here.
*
*/

#include            "global.h"
#include            "syscalls.h"
#include            "protos.h"
#include            "string.h"

#include            "userdefs.h"

extern UINT16        *Z502_PAGE_TBL_ADDR;

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
        printf("Priority: %d\n", ptrCheck->p_priority);
        printf("State: %d\t", ptrCheck->p_state);
        printf("MSGState: %d\t", ptrCheck->msg_state);
        printf("Disk: %d\n", ptrCheck->disk);
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
//Print Table Manager
void printTable( void ){
    FRAMETABLE_t * ptrCheck = pageList;
    printf("\n------------------FRAME TABLE QUEUE-----------------\n");        
    while (ptrCheck != NULL){
        if( ptrCheck->page != -1){
            printf("ID: %d\t", ptrCheck->p_id);
            printf("Frame: %d\t", ptrCheck->frame);
            printf("Page: %d   \t", ptrCheck->page);
            printf("Time: %d\n", ptrCheck->refTime);            
        }
        ptrCheck = ptrCheck->next;
    }    
}
//Print Shadow Table
void printShadow( void ){
    SHADOWTABLE_t *ptrCheck = shadowList;
    printf("\n----------------SHADWOW TABLE QUEUE----------------\n");
    while(ptrCheck != NULL){
	printf("ID: %d\t", ptrCheck->p_id);
	printf("page: %d\t", ptrCheck->page);
	printf("frame: %d\t", ptrCheck->frame);
	printf("sector: %d\n", ptrCheck->sector);
	printf("------------------------------------------------\n");
	ptrCheck = ptrCheck->next;
	}
}
// Helper function used to only print to screen
//    if the DEBUGFLAG is set in the userdefs.h file.
void debugPrint ( char * toprint ){
    if (DEBUGFLAG){
        printf("%s\n", toprint);
    }
}
//Print memory usage
void printMemory ( void  ){
    FRAMETABLE_t     *ptrCheck;
    ptrCheck = pageList;
    while( ptrCheck != NULL ){
        if( ptrCheck->page != -1 ){
            MP_setup( ptrCheck->frame, ptrCheck->p_id, 
            ptrCheck->page, (*Z502_PAGE_TBL_ADDR & 0xE000) >> 13 );
        }
        ptrCheck = ptrCheck->next;
    }
    MP_print_line(); 
}
void printState ( char action[SP_LENGTH_OF_ACTION]){
    PCB_t *ptrCheck = pidList;

    CALL( SP_setup( SP_TIME_MODE, get_currentTime() ) );
    CALL( SP_setup_action( SP_ACTION_MODE, action ) );

    CALL( SP_setup( SP_TARGET_MODE, current_PCB->p_id ) );
    while(ptrCheck!= NULL){
        CALL( SP_setup( SP_RUNNING_MODE, current_PCB->p_id ) );
        switch(ptrCheck->p_state){
            case(NEW_STATE):
                CALL( SP_setup( SP_NEW_MODE, current_PCB->p_id  ) );
                break;
            case(READY_STATE):
                CALL( SP_setup( SP_READY_MODE, ptrCheck->p_id  ) ); 
                break;
            case(WAITING_STATE):
                CALL( SP_setup( SP_WAITING_MODE, ptrCheck->p_id ) );
                break;
            case(SUSPENDED_STATE):
                CALL( SP_setup( SP_SUSPENDED_MODE, ptrCheck->p_id ) );
                break;
        }
        ptrCheck = ptrCheck->next;
    }
    CALL( SP_print_header() );
    CALL( SP_print_line() );
}

