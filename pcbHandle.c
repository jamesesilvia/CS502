/*
*	This file contains all routines that handles the PCB 
*	upriorities. And helper functions to change states
*
*	CHANGE PRIORITY
*
*/
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * CHANGE PRIORITY
 *
 *	This routine updates the priority of a process on the Ready Queue.
 *	It calls the sub-routine updatePriority to change the process ID's
 *	priority. If any error occurs, this function returns FAILURE to the OS.
 *	
 *	After the priority is changed, the Ready Queue is sorted based on
 *	the new priority, and a new process is switched to if the highest
 *	priority function is different.
 *
 */
void change_Priority( INT32 process_ID, INT32 new_priority, INT32 *error ){
	PCB_t * switchPCB;
	INT32	status;

	//New priority exceeds limit
	if (new_priority > MAX_PRIO){
		debugPrint("CHANGE PRIORITY ERROR: NEW PRIORITY EXCEEDS MAX");
		(*error) = ERR_BAD_PARAM;
		return;
	}
	//if pid -1, UPDATE SELF
	if ( process_ID == -1 ){
		//UPDATE SELF, or return error
		process_ID = current_PCB->p_id;
		CALL( status = updatePriority(process_ID, new_priority) );
		if (status == 1) (*error) = ERR_SUCCESS;
		//ERROR
		else{
			debugPrint("CHANGE PRIORITY ERROR: UPDATING SELF FAILED");
			(*error) = ERR_BAD_PARAM;
			return;
		}
	}
	//else, update PID
	else{
		CALL( status = updatePriority(process_ID, new_priority) );
		if (status == 1) (*error) = ERR_SUCCESS;
		//ERROR
		else{
			debugPrint("CHANGE PRIORITY ERROR: UPDATING ID FAILED");
			(*error) = ERR_BAD_PARAM;
			return;
		}
	}

	//SORT the Ready Queue based on the new priorities
	ZCALL( lockReady() );
	CALL( ready_sort() );
	ZCALL( unlockReady() );

	//State Print Out
	if (PRIORITYpo && DEBUGFLAG){
		SP_setup( SP_TARGET_MODE, process_ID );
		SP_setup( SP_PRIORITY_MODE, new_priority );
		SP_print_header();
       	SP_print_line();
       	printReady();
	}

	//GET READY PCB TO RUN
	//If the ready PCB is already running, return
	//Else switch to the new process
	CALL( switchPCB = get_readyPCB() );
	if (switchPCB == NULL) EVENT_IDLE();
	else if (switchPCB->p_id == current_PCB->p_id) return;
	if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
}
//Helper function to update the priority of a process ID
//Returns -1 if the ID does not exist
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

 //Helper functions used to update states

 //Update to a Waiting State
PCB_t *ready_to_Wait ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;
	PCB_t * temp;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = WAITING_STATE;
			temp = ptrDel;
			return temp;
		}
		ptrDel = ptrDel->next;
	}
	//No ID in PCB List
	return NULL;
}
//Update to a Ready State
void wait_to_Ready ( INT32 remove_id ){
	PCB_t * ptrDel = pidList;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			ptrDel->p_state = READY_STATE;
			return;
		}
		ptrDel = ptrDel->next;
	}
	//No ID in PCB List
	return;
}
//Change READY state to RUNNING
//When a new process is switched to
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
