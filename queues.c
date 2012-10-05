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

//	printf("REMOVED ID %d FROM READY", remove_id);
	CALL( temp = rm_from_Queue(&pidList, remove_id) );
	if ( (temp == NULL) && (total_pid != 0) ){
		total_pid++;
		ZCALL( unlockReady() );
		return -1;
	}
	else{
//		printf("REMOVED %s", temp->p_name);
		ZCALL( unlockReady() );
		return 1;
	}
}
void rm_from_timerQueue (  PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t *temp;
	
	ZCALL( lockTimer() );
	CALL( temp = rm_from_Queue(ptrFirst, remove_id) );
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE TIMER");
	ZCALL( unlockTimer() );
}
PCB_t *rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t * ptrDel = *ptrFirst;
	PCB_t * ptrPrev = NULL;

//	printf("NEED TO REMOVE ID: %d\n", remove_id);
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
//				printf("Middle - Name: %s", ptrDel->p_name);
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
/*
 * Moving Queues
 */
void readyQueue_to_timerQueue( INT32 remove_id ){
	PCB_t *temp;

	ZCALL( lockReady() );
	CALL( temp = ready_to_Wait(remove_id) );
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE (READY TO TIMER)");
	ZCALL( unlockReady() );
	
	if (temp != NULL){	
		CALL( add_to_timerQueue(&timerList, temp) );
//		print_queues(&timerList);
	}
}
void timerQueue_to_readyQueue( INT32 remove_id ){
	PCB_t *temp;

	CALL( temp = rm_from_Queue(&timerList, remove_id) );
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE (TIMER TO READY)");

	ZCALL( lockReady() );
	CALL( wait_to_Ready(remove_id) );
	ZCALL( unlockReady() );
}

// INT32 get_first_ID ( PCB_t ** ptrFirst ){
	// lockBoth();
	// PCB_t *ptrCheck = *ptrFirst;

	// while (ptrCheck != NULL){
			// ptrCheck = ptrCheck->next;
		// }
	// unlockBoth();
	// return ptrCheck->p_id;
// }
// PCB_t * get_firstPCB(PCB_t ** ptrFirst){
	// lockBoth();
	// PCB_t *ptrCheck = *ptrFirst;
	// PCB_t *ptrPrev = NULL;

	// while (ptrCheck != NULL){
		// ptrPrev = ptrCheck;
		// ptrCheck = ptrCheck->next;
	// }
	// unlockBoth();
	// return ptrPrev;
// }
PCB_t * get_readyPCB( void ){
	
//	CALL ( printf("GET READY PCB\n") );
	ZCALL ( lockReady() );
	PCB_t *ptrCheck = pidList;
	PCB_t *ptrPrev = NULL;

	while (ptrCheck != NULL){
//		CALL ( printf("END NAME %s", ptrCheck->p_name) );
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	
	ptrCheck = ptrPrev;
	while (ptrCheck != NULL){
//		printf("ptrCheck NAME %s\n", ptrCheck->p_name);
		if (ptrCheck->p_state == READY_STATE){
			ZCALL( unlockReady() );
//			printf("READY ptrCheck Name %s\n", ptrCheck->p_name);
			return ptrCheck;
		}
		ptrCheck = ptrCheck->prev;	
	}
	ZCALL( unlockReady() );
	return NULL;
}
//Timer Handling
INT32 wake_timerList ( INT32 currentTime ){
	ZCALL( lockTimer() );
	PCB_t *ptrCheck = timerList;
	PCB_t *ptrPrev = NULL;
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
	return (ptrCheck->p_time - get_currentTime() + 500);
}


