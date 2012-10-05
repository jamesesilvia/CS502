#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * GET PROCESS ID
 */
INT32 get_PCB_ID(PCB_t ** ptrFirst, char *name, INT32 *process_ID, INT32 *error){
	ZCALL( lockReady() );
//	printf("GET PCB ID %s\n", name);

	if (strcmp("", name) == 0){
		(*process_ID) = current_PCB->p_id;
		(*error) = ERR_SUCCESS;
		ZCALL( unlockReady() );
		return 0;
	}
	PCB_t *ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		if (strcmp(ptrCheck->p_name, name) == 0){
			(*process_ID) = ptrCheck->p_id;
			(*error) = ERR_SUCCESS;
			ZCALL( unlockReady() );
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	(*error) = ERR_BAD_PARAM;
	ZCALL( unlockReady() );
	return -1;
}

/*
 * TERMINATE PROCESS
 */
void terminate_Process ( INT32 process_ID, INT32 *error ){
	PCB_t * switchPCB;

	INT32	status;
	if (total_pid <= 0) ZCALL( Z502_HALT() );

	//if pid is -1; terminate self
	if ( process_ID == -1 ){
		//printf("\nTerminate self\n");

		CALL( status = rm_from_readyQueue(current_PCB->p_id) );
		if (status) (*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;
		if (total_pid > 0){
			CALL( switchPCB = get_readyPCB() );
			if (switchPCB == NULL) ZCALL( Z502_IDLE() );
			if (switchPCB !=NULL) CALL( switch_Savecontext(switchPCB) );
		}
		else CALL ( Z502_HALT() );
	}
	//if pid -2; terminate self and any child
	else if ( process_ID == -2 ){
		//printf("\nTerminate self and children\n");
		CALL( rm_children(&pidList, current_PCB->p_id) );

		CALL( status = rm_from_readyQueue(current_PCB->p_id) );
		if (status) (*error) = ERR_SUCCESS;
		else 	(*error) = ERR_BAD_PARAM;
		if (total_pid > 0){
			CALL( switchPCB = get_readyPCB() );
			if (switchPCB == NULL) ZCALL( Z502_IDLE() );
			if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
		}
		else ZCALL( Z502_HALT() );
	}
	//remove pid from pidList
	else{
		CALL( status = rm_from_readyQueue(process_ID) );
		if (status)	(*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;
		if (total_pid <= 0){
			ZCALL ( Z502_HALT() );
		}
	}
}
//Helper function for terminate process
void rm_children ( PCB_t ** ptrFirst, INT32 process_ID ){
	PCB_t * ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		if ( ptrCheck->p_parent == process_ID ){
//			printf("ptrCheck Parent Name %s\n", ptrCheck->p_name);
			CALL( rm_from_readyQueue( ptrCheck->p_id ) );
		}
		ptrCheck = ptrCheck->next;
	}
}

/*
 * SUSPEND PROCESS
 */
void suspend_Process ( INT32 process_ID, INT32 *error ){
	PCB_t * switchPCB;
	INT32	status;

	//if pid -1, suspend self
	if ( process_ID == -1 ){
		//RUNNING TO SUSPEND STATE
		CALL( status = ready_to_Suspend(current_PCB->p_id) );
		if (status) (*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;

		//GET READY PCB TO RUN
		CALL( switchPCB = get_readyPCB() );
		if (switchPCB == NULL) ZCALL( Z502_IDLE() );
		if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
	}
	else{
		CALL( status = ready_to_Suspend(process_ID) );
		if (status) (*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;
	}
}
INT32 ready_to_Suspend ( INT32 process_ID ){
	ZCALL( lockReady() );
	PCB_t *ptrCheck = pidList;

	while ( ptrCheck != NULL ){
		if (ptrCheck->p_id == process_ID){
			ptrCheck->p_state = SUSPENDED_STATE;
			ZCALL( unlockReady() );
			return 1;
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	return -1;
}
/*
 * RESUME PROCESS
 */
void resume_Process ( INT32 process_ID, INT32 *error ){
	ZCALL( lockReady() );
	PCB_t *ptrCheck = pidList;

	while ( ptrCheck != NULL ){
		if (ptrCheck->p_id == process_ID){
			if (ptrCheck->p_state == SUSPENDED_STATE){
				ptrCheck->p_state = READY_STATE;
				ZCALL( unlockReady() );
				(*error) = ERR_SUCCESS;
				return;
			}
		}
		ptrCheck = ptrCheck->next;
	}
	ZCALL( unlockReady() );
	(*error) = ERR_BAD_PARAM;
	return;
}
