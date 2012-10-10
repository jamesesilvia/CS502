#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * Adding to Queues
 */
void add_to_readyQueue ( PCB_t **ptrFirst, PCB_t *entry ){
	total_pid++;
	entry->p_state = READY_STATE;
	ZCALL( lockReady() );
	CALL( add_to_Queue(ptrFirst, entry) );
	CALL( ready_sort() );
	ZCALL( unlockReady() );
}
void add_to_timerQueue( PCB_t **ptrFirst, PCB_t *entry ){
	PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));
	memcpy(PCB, entry, sizeof(PCB_t));
	PCB->next = NULL;
	PCB->prev = NULL;
	
	ZCALL( lockTimer() );
	CALL( add_to_Queue(ptrFirst, PCB) );
	CALL( timer_sort() );
	ZCALL( unlockTimer() );
}
void add_to_eventQueue ( INT32 *device_id, INT32 *status ){
	EVENT_t *event = (EVENT_t *)(malloc(sizeof(EVENT_t)));
	event->device_ID = *device_id;
	event->Status = *status;
	event->next = NULL;
	
	event_count++;
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
INT32 add_to_msgQueue ( MSG_t *entry ){
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
 */
INT32 rm_from_readyQueue ( INT32 remove_id ){
	PCB_t *temp;
	total_pid--;
	ZCALL( lockReady() );

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
	
	ZCALL( lockTimer() );
	CALL( temp = rm_from_Queue(ptrFirst, remove_id) );
	ZCALL( unlockTimer() );
}
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
*/
PCB_t * get_readyPCB( void ){
	
	ZCALL ( lockReady() );
	PCB_t *ptrCheck = pidList;
	PCB_t *ptrPrev = NULL;
	PCB_t *ptrReturn = NULL;

	while (ptrCheck != NULL){
		if (ptrCheck->p_state == READY_STATE){
			if (ptrReturn==NULL) ptrReturn = ptrCheck;
			else if (ptrReturn->p_priority <= ptrCheck->p_priority){
				ptrReturn = ptrCheck;
			}		
		}
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	return ptrReturn;
}
//Timer Handling
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
		}
		ptrCheck = ptrCheck->prev;
	}
	ZCALL( unlockTimer() );
	return count;
}
//Gets sleeptime from timerList
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
//check if ID exists in pidList
check_pid_ID ( INT32 check_ID ){
	ZCALL( lockReady() );
	PCB_t 	*ptrCheck = pidList;

	while(ptrCheck != NULL){
		//ID Exists
		if (ptrCheck->p_id == check_ID){
			ZCALL( unlockReady() );
			return 1;
		}
		ptrCheck = ptrCheck->next;
	}
	//ID does not exist
	ZCALL ( unlockReady() );
	return -1;
}

