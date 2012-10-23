/*
*	This file handles all the queues
*
*	READY QUEUE
*	TIMER QUEUE
*	EVENT QUEUE
*	MESSAGE LIST
*
*	ADDING to Queues
*	REMOVING from Queues
*	ADDING to Message List
*	REMOVING from Message List
*	
*	MOVING from Ready to Timer Queue
*	MOVING from Timer to Ready Queue
*
*	This file also includes getting the Ready PCB with the
*	lowest priority
*		PCB_t * get_readyPCB( void )
*
*	As well as waking up items from the Timer Queue
*	and moving them to the readey Queue.\
*		INT32 wake_timerList ( INT32 currentTime  )
*
*	checkTimer retrieves the sleepTime from Timer Queue
*/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * Adding to Queues
 *
 *	Used to add to Queues
 *	The function names are self-explanatory
 */
void add_to_readyQueue ( PCB_t **ptrFirst, PCB_t *entry ){
	total_pid++;
	//Set to Ready State
	entry->p_state = READY_STATE;
	//Add to Ready Queue
	ZCALL( lockReady() );
	CALL( add_to_Queue(ptrFirst, entry) );
	//Sort the Ready Queue
	CALL( ready_sort() );
	ZCALL( unlockReady() );
}
void add_to_timerQueue( PCB_t **ptrFirst, PCB_t *entry ){
	//Set space for new timer PCB
	PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));
	memcpy(PCB, entry, sizeof(PCB_t));
	PCB->next = NULL;
	PCB->prev = NULL;
	
	//Add and sort Timer Queue based on wakeUp Time
	ZCALL( lockTimer() );
	CALL( add_to_Queue(ptrFirst, PCB) );
	CALL( timer_sort() );
	ZCALL( unlockTimer() );
}
void add_to_eventQueue ( INT32 *device_id, INT32 *status ){
	//Set space for the new event PCB
	EVENT_t *event = (EVENT_t *)(malloc(sizeof(EVENT_t)));
	event->device_ID = *device_id;
	event->Status = *status;
	event->next = NULL;
	
	// Increase global event_count
	event_count++;
	// Different typedef, could not use default add_to_Queue
	EVENT_t * ptrCheck = eventList;
	if (ptrCheck == NULL){
		eventList = event;
		return;
	}	
	while(ptrCheck->next != NULL){
		ptrCheck = ptrCheck->next;
	}
	ptrCheck->next = event;
}
INT32 add_to_Outbox ( MSG_t *entry ){
	//Different typedef, could not use default add_to_Queue

	(current_PCB->msg_count)++;
	if(current_PCB->msg_count > MAX_MSG_COUNT){
		return -1;
	}

	MSG_t *toSend = current_PCB->Outbox;
	//if First Message
	if (toSend == NULL){
		current_PCB->Outbox = entry;
		return 1;
	}
	//Otherwise append to end of Outbox
	while (toSend->next != NULL){
		toSend = toSend->next;
	}
	toSend->next = entry;
	return 1;
}
void add_to_Inbox ( PCB_t *dest, MSG_t *msgRecv ){
	//Different typedef, could not use default add_to_Queue

	if (msgRecv == NULL) return;
	//Create the new Inbox Message
	MSG_t *MSG = (MSG_t *)(malloc(sizeof(MSG_t)));
 	MSG->dest_ID = msgRecv->dest_ID;
 	MSG->src_ID = msgRecv->src_ID;
 	MSG->Length = msgRecv->Length;
 	memset(MSG->message, 0, MAX_MSG+1);
	strcpy(MSG->message, msgRecv->message);
	MSG->next = NULL;

	MSG_t *Inbox = dest->Inbox;

	//If First Message
	if ( Inbox == NULL){
		dest->Inbox = MSG;
		return;
	}
	//Otherwise append to end of Outbox
	while (Inbox->next != NULL){
		Inbox = Inbox->next;
	}
	Inbox->next = MSG;
}
//Helper function for timer and ready Queues
INT32 add_to_Queue( PCB_t **ptrFirst, PCB_t * entry ){
	//First Case
	if ( *ptrFirst == NULL){
		(*ptrFirst) = entry;
		return 1;
	}
	//Add to start of list
	else{
		(*ptrFirst)->prev = entry;
		entry->next = (*ptrFirst);
		(*ptrFirst) = entry;
		return 1;
	}
	return 0;
}
/*
 * Removing from Queues
 *
 *	Used to remove from all queues
 *	The function names are self-explanatory
 */
INT32 rm_from_readyQueue ( INT32 remove_id ){
	PCB_t *temp;
	total_pid--;
	ZCALL( lockReady() );

	//Use default rm_from_Queue for PCB_t
	CALL( temp = rm_from_Queue(&pidList, remove_id) );
	if ( (temp == NULL) && (total_pid != 0) ){
		total_pid++;
		ZCALL( unlockReady() );
		return -1;
	}
	else{
		ZCALL( unlockReady() );
		return 1;
	}
}
void rm_from_timerQueue (  PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t *temp;
	
	//Use default rm_from_Queue for PCB_t
	ZCALL( lockTimer() );
	CALL( temp = rm_from_Queue(ptrFirst, remove_id) );
	ZCALL( unlockTimer() );
}
//Default remove from Queue function used for Ready and Timer Queue
PCB_t *rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t * ptrDel = *ptrFirst;
	PCB_t * ptrPrev = NULL;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			//First ID
			if ( ptrPrev == NULL){
				(*ptrFirst) = ptrDel->next;
				return ptrDel;
			}
			//Last ID
			else if (ptrDel->next == NULL){
				ptrPrev->next = NULL;
				return ptrDel;
			}
			//Middle ID
			else{
				PCB_t * ptrNext = ptrDel->next;
				ptrPrev->next = ptrDel->next;
				ptrNext->prev = ptrDel->prev;
				return ptrDel;
			}
		}
		ptrPrev = ptrDel;
		ptrDel = ptrDel->next;
	}
	//No ID in PCB List
	return NULL;
}
//Remove item from the eventQueue based on ID
void rm_from_eventQueue ( INT32 remove_id ){
	EVENT_t *ptrDel = eventList;
	EVENT_t *ptrPrev = NULL;
	
	while(ptrDel != NULL){
		if(ptrDel->device_ID == remove_id){
			event_count--;
			//First ID
			if(ptrPrev == NULL){
				eventList = ptrDel->next;
				return;
			}
			//Last ID
			else if (ptrDel->next == NULL){
				ptrPrev->next = NULL;
				return;
			}
			//Midle ID
			else{
				ptrPrev->next = ptrDel->next;
				return;			
			}		
		}
		ptrPrev = ptrDel;
		ptrDel = ptrDel->next;		
	}	
}
/*
 * Moving Queues
 *
 *	These functions take advantange of the above functions,
 *	but allowed me to create functions that moved processes
 *	from one queue to another.
 *
 *		Ready Queue to Timer Queue
 *		Timer Queue to Ready Queue
 */
void readyQueue_to_timerQueue( INT32 remove_id ){
	PCB_t *temp;

	ZCALL( lockReady() );
	CALL( temp = ready_to_Wait(remove_id) );
	ZCALL( unlockReady() );
	
	if (temp != NULL){	
		CALL( add_to_timerQueue(&timerList, temp) );
	}
}
void timerQueue_to_readyQueue( INT32 remove_id ){
	PCB_t *temp;

	CALL( temp = rm_from_Queue(&timerList, remove_id) );

	ZCALL( lockReady() );
	CALL( wait_to_Ready(remove_id) );
	ZCALL( unlockReady() );
}
/*
* Get the ready PCB with lowest priority
*
*	Cycle through the Ready List and find the
*	PCB ready to run with the lowest priority.
*
*/
PCB_t * get_readyPCB( void ){
	
	ZCALL ( lockReady() );
	PCB_t *ptrCheck = pidList;
	PCB_t *ptrPrev = NULL;
	PCB_t *ptrReturn = NULL;

	while (ptrCheck != NULL){
		if (ptrCheck->p_state == READY_STATE){
			if (ptrReturn==NULL) ptrReturn = ptrCheck;
			//If next PCB has lower or == priority, its the new ready PCB
			else if (ptrReturn->p_priority >= ptrCheck->p_priority){
				ptrReturn = ptrCheck;
			}		
		}
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	return ptrReturn;
}
/*
* Wake up items from the Timer List
*
*	This function will wake up all items on the Timer List
*	by moving them to the Ready Queue. The processes are wokenUp
*	based on the currentTime and their wakeUp time in the PCB
*
*/
INT32 wake_timerList ( INT32 currentTime ){
	ZCALL( lockTimer() );
	PCB_t 	*ptrCheck = timerList;
	PCB_t 	*ptrPrev = NULL;
	INT32	count = 0;
	
	while (ptrCheck != NULL){
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	ptrCheck = ptrPrev;
	while (ptrCheck != NULL){
		if (ptrCheck->p_time <= currentTime){
			CALL( timerQueue_to_readyQueue(ptrCheck->p_id) );
			count++;

			//State Printout
			if ( WAKEUPpo && DEBUGFLAG){
				SP_setup( SP_WAKEUP_MODE, ptrCheck->p_id );
        		SP_print_header();
       			SP_print_line();
			}

		}
		ptrCheck = ptrCheck->prev;
	}
	ZCALL( unlockTimer() );
	return count;
}
/* 
* Get sleepTime from Timer Queue
*
*	Returns the difference between the
*	smallest wakeUp time, and the currentTime.
*	
*	This value is used to restart the Timer
*/
INT32 checkTimer ( INT32 currentTime ){
	ZCALL( lockTimer() );

	PCB_t *ptrCheck = timerList;
	//No more items on timerList
	if (ptrCheck == NULL){
		ZCALL( unlockTimer() );
		return -1;
	}
	//New sleeptime for Timer
	unlockTimer();
	return (ptrCheck->p_time - currentTime);
}
// Check if ID exists in pidList
// Return -1 id ID does not exist
check_pid_ID ( INT32 check_ID ){
	PCB_t 	*ptrCheck = pidList;

	while(ptrCheck != NULL){
		//ID Exists
		if (ptrCheck->p_id == check_ID){
			return 1;
		}
		ptrCheck = ptrCheck->next;
	}
	//ID does not exist
	return -1;
}

