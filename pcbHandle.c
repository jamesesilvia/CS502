#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * Change Priority
 */
void change_Priority( INT32 process_ID, INT32 new_priority, INT32 *error ){

	PCB_t * switchPCB;
	INT32	status;

	//if pid -1, UPDATE SELF
	if ( process_ID == -1 ){
		//UPDATE SELF, or return error
		CALL( status = updatePriority(current_PCB->p_id, new_priority) );
		if (status) (*error) = ERR_SUCCESS;
		else{
			(*error) = ERR_BAD_PARAM;
			return;
		}
	}
	//else, update PID
	else{
		CALL( status = updatePriority(process_ID, new_priority) );
		if (status) (*error) = ERR_SUCCESS;
		else{
			(*error) = ERR_BAD_PARAM;
			return;
		}
	}
	ZCALL( lockReady() );
	CALL( ready_sort() );
//	printReady();
	ZCALL( unlockReady() );

	//GET READY PCB TO RUN
	CALL( switchPCB = get_readyPCB() );
	if (switchPCB == NULL) Z502_IDLE();
	else if (switchPCB->p_id == current_PCB->p_id) return;
	if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
}
INT32 updatePriority ( INT32 process_ID, INT32 new_priority ){
	ZCALL( lockReady() );
	PCB_t *ptrCheck = pidList;

	while ( ptrCheck != NULL ){
		if (ptrCheck->p_id == process_ID){
			ptrCheck->p_priority= new_priority;
			ZCALL( unlockReady() );
			return 1;
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	return -1;
}
/*
 * Change States of PCB
 */
PCB_t *ready_to_Wait ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;
	PCB_t * temp;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = WAITING_STATE;
//			printf("CHANGE STATE %d", ptrDel->p_state);
			temp = ptrDel;
			return temp;
		}
		ptrDel = ptrDel->next;
	}
	//No ID in PCB List
	return NULL;
}
void wait_to_Ready ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = READY_STATE;
//			printf("\nCHANGE STATE %d NAME %s\n", ptrDel->p_state, ptrDel->p_name);
			return;
		}
		ptrDel = ptrDel->next;
	}
	//No ID in PCB List
	return;
}
void ready_to_Running ( void ){
	ZCALL( lockReady() );
	PCB_t * ptrDel = pidList;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == current_PCB->p_id){
			ptrDel->p_state = RUNNING_STATE;
		}
		else if (ptrDel->p_state == RUNNING_STATE){
			ptrDel->p_state = READY_STATE;
		}
		ptrDel = ptrDel->next;
	}
	ZCALL( unlockReady() );
	return;
}
