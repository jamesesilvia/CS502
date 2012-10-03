#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

// Added 9/4/2012
#include			"userdefs.h"

/*
 * Adding to Queues
 */
void add_to_readyQueue ( PCB_t **ptrFirst, PCB_t *entry ){
	total_pid++;
	entry->p_state = READY_STATE;
	lockReady();
	add_to_Queue(ptrFirst, entry);
//	priority_sort(ptrFirst);
//	print_queues(&pidList);
	unlockReady();
}
void add_to_timerQueue( PCB_t **ptrFirst, PCB_t *entry ){
	PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));
	memcpy(PCB, entry, sizeof(PCB_t));
//	printf("\nName %s\t ID %s\n", entry->p_name, entry->p_id);
	PCB->next = NULL;
	PCB->prev = NULL;
	lockTimer();
	add_to_Queue(ptrFirst, PCB);
	timer_sort();
	unlockTimer();
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
	lockReady();

	printf("REMOVED ID %d FROM READY", remove_id);
	temp = rm_from_Queue(&pidList, remove_id);
	if (temp == NULL){
//		printf("RETURNED NULL IN REMOVE READY");
		unlockReady();
		return -1;
	}
	else{
//		printf("REMOVED %s", temp->p_name);
		unlockReady();
		return 1;
	}
}
void rm_from_timerQueue (  PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t *temp;
	lockTimer();
	temp = rm_from_Queue(ptrFirst, remove_id);
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE TIMER");
	unlockTimer();
}
PCB_t *rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id ){
	PCB_t * ptrDel = *ptrFirst;
	PCB_t * ptrPrev = NULL;

//	printf("NEED TO REMOVE ID: %d\n", remove_id);
//	print_queues(&pidList);
	while ( ptrDel != NULL ){
//		printf("Name: %s ID: %d\n", ptrDel->p_name, ptrDel->p_id);
		if (ptrDel->p_id == remove_id){
			//First ID
			if ( ptrPrev == NULL){

				(*ptrFirst) = ptrDel->next;
//				(*ptrFirst)->prev = NULL;

//				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );
//				printf("First - Name: %s", ptrDel->p_name);

//				ptrDel->next = NULL;
//				ptrDel->prev = NULL;
				return ptrDel;
			}
			//Last ID
			else if (ptrDel->next == NULL){
				ptrPrev->next = NULL;

//				printf("Last - Name: %s", ptrDel->p_name);
//				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );

//				ptrDel->next = NULL;
//				ptrDel->prev = NULL;
				return ptrDel;

			}
			//Middle ID
			else{
//				printf("Middle - Name: %s", ptrDel->p_name);
				PCB_t * ptrNext = ptrDel->next;
				ptrPrev->next = ptrDel->next;
				ptrNext->prev = ptrDel->prev;
//				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );

//				ptrDel->next = NULL;
//				ptrDel->prev = NULL;
				return ptrDel;
			}
		}
		ptrPrev = ptrDel;
		ptrDel = ptrDel->next;;
	}
	//No ID in PCB List
	return NULL;
}
/*
 * Moving Queues
 */
void readyQueue_to_timerQueue( INT32 remove_id ){
	PCB_t *temp;

	lockReady();
	temp = ready_to_Wait(remove_id);
//	temp = rm_from_Queue(&pidList, remove_id);
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE (READY TO TIMER)");
	unlockReady();
	
	if (temp != NULL){	
		lockTimer();
		add_to_timerQueue(&timerList, temp);
//		print_queues(&timerList);
		unlockTimer();
	}
}
void timerQueue_to_readyQueue( INT32 remove_id ){
	PCB_t *temp;

	temp = rm_from_Queue(&timerList, remove_id);
//	if (temp == NULL) printf("RETURNED NULL IN REMOVE (TIMER TO READY)");

	lockReady();
	wait_to_Ready(remove_id);
	unlockReady();
}
PCB_t *ready_to_Wait ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;
	PCB_t * temp;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = WAITING_STATE;
//			printf("CHANGE STATE %d", ptrDel->p_state);
			temp = ptrDel;
//			temp->next = NULL;
//			temp->prev = NULL;
			return temp;
		}
		ptrDel = ptrDel->next;;
	}
	//No ID in PCB List
	return NULL;
}
void wait_to_Ready ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = READY_STATE;
//			printf("\nCHANGE STATE %d\n", ptrDel->p_state);
			return;
		}
		ptrDel = ptrDel->next;;
	}
	//No ID in PCB List
	return;
}

INT32 check_name( PCB_t **ptrFirst, char *name ){
	lockBoth();
	PCB_t *ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
		if(strcmp(ptrCheck->p_name, name) == 0){

			unlockBoth();
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	unlockBoth();
	return 1;
}

INT32 get_PCB_ID(PCB_t ** ptrFirst, char *name, INT32 *process_ID, INT32 *error){
	lockBoth();
	printf("GET PCB ID %s\n", name);

	if (strcmp("", name) == 0){
		(*process_ID) = current_PCB->p_id;
		(*error) = ERR_SUCCESS;
//		printf("PROCESS ID: %d", current_PCB->p_id);
		unlockBoth();
		return 0;
	}
	PCB_t *ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		if (strcmp(ptrCheck->p_name, name) == 0){
//			printf("PROCESS ID: %d", ptrCheck->p_id);
			(*process_ID) = ptrCheck->p_id;
			(*error) = ERR_SUCCESS;
//			printf("PROCESS ID: %d", current_PCB->p_id);
//			print_queues(ptrFirst);
//			ZCALL( Z502_HALT() );

			unlockBoth();
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	(*error) = ERR_BAD_PARAM;
//	printf("ERROR BAD PARAM");
	unlockBoth();
	return -1;
}

INT32 get_first_ID ( PCB_t ** ptrFirst ){
	lockBoth();
	PCB_t *ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
			ptrCheck = ptrCheck->next;
		}
	unlockBoth();
	return ptrCheck->p_id;
}

PCB_t * get_firstPCB(PCB_t ** ptrFirst){
	lockBoth();
	PCB_t *ptrCheck = *ptrFirst;
	PCB_t *ptrPrev = NULL;

	while (ptrCheck != NULL){
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	unlockBoth();
	return ptrPrev;
}
PCB_t * get_readyPCB( void ){
	
//	CALL ( printf("GET READY PCB\n") );
	lockBoth();
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
			unlockBoth();
//			printf("READY ptrCheck Name %s\n", ptrCheck->p_name);
			return ptrCheck;
		}
		ptrCheck = ptrCheck->prev;	
	}
	unlockBoth();
	return NULL;
}
INT32 wake_timerList ( INT32 currentTime ){
	print_queues(&timerList);
	lockTimer();
	PCB_t *ptrCheck = timerList;
	PCB_t *ptrPrev = NULL;
	INT32	count = 0;
	
	while (ptrCheck != NULL){
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
	ptrCheck = ptrPrev;
	while (ptrCheck != NULL){
		if (ptrCheck->p_time >= currentTime){
			timerQueue_to_readyQueue ( ptrCheck->p_id );
			count++;
		}
		ptrCheck = ptrCheck->prev;
	}
	unlockTimer();
	CALL( printf("WOKE UP %d\n", count) );
	return count;
}

void priority_sort( PCB_t ** ptrFirst ){
	PCB_t * ptrCheck = *ptrFirst;
	PCB_t * temp = NULL;

	if ( ptrCheck->next == NULL ) return;

	PCB_t * ptrNext = ptrCheck->next;

	while ( ptrNext != NULL ){
		if ( ptrCheck->p_priority < ptrNext->p_priority ){
			temp = ptrCheck;
			ptrCheck->prev = ptrNext;
			ptrCheck->next = ptrNext->next;
			ptrNext->prev = temp->prev;
			ptrNext->next = temp;
		}
		else return;
		ptrNext = ptrCheck->next;
	}
	return;
}

void timer_sort( void ){
	PCB_t * ptrCheck = timerList;
	PCB_t * ptrNext = NULL;
	PCB_t * temp = NULL;

//	for (; ptrCheck->next != NULL; ptrCheck = ptrCheck->next ){
//		for (ptrNext = ptrCheck->next; ptrNext != NULL; ptrNext = ptrNext->next){
//			printf("STUCK%d %d",ptrCheck->p_id, ptrNext->p_id);
//			if ( ptrCheck->p_time >= ptrNext->p_time ){
////				printf("\nptrNext->next ID\n %d", ptrNext->p_id);
////				Z502_HALT();
//				temp = ptrCheck;
//				ptrCheck->prev = ptrNext;
//				ptrCheck->next = ptrNext->next;
//				ptrNext->prev = temp->prev;
//				ptrNext->next = temp;
//			}
//
//		}
//	}
//	return;

	if ( ptrCheck->next == NULL ) return;

	 ptrNext = ptrCheck->next;

	while ( ptrNext != NULL ){
		if ( ptrCheck->p_time < ptrNext->p_time ){
//			printf("STUCK%d %d",ptrCheck->p_id, ptrNext->p_id);
			temp = ptrCheck;
			ptrCheck->prev = ptrNext;
			ptrCheck->next = ptrNext->next;
			ptrNext->prev = temp->prev;
			ptrNext->next = temp;
		}
		ptrCheck = ptrNext;
		ptrNext = ptrCheck->next;	
	}
	return;
}
void print_queues ( PCB_t **ptrFirst ){
	lockBoth();
	if (*ptrFirst == NULL) return;
	PCB_t * ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		printf("------------------------------\n");
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("Time: %d\t", ptrCheck->p_time);
		printf("Priority: %d\t", ptrCheck->p_priority);
		printf("State: %d\n", ptrCheck->p_state);
		ptrCheck = ptrCheck->next;
	}
	unlockBoth();
	return;
}


