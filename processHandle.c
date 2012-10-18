#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

/*
 * GET PROCESS ID
 *
 * 	This routine checks the Ready Queue for a name that
 *	matches the name passed to this routine. If the input
 *	to the routine is "", then the currently running process'
 *	ID is returned. Success or fail is passed to the OS, and
 *	also -1 is returned from the function if the name does 
 *	not exist.
 *
 */
INT32 get_PCB_ID(PCB_t ** ptrFirst, char *name, INT32 *process_ID, INT32 *error){
	ZCALL( lockReady() );

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
 *
 *	This routine takes the process ID for a process to be
 *	terminated. If the ID = -1, then the currently running
 *	process is terminated. If ID = -2, then any processes
 *	that have a parent ID of the current process, as well
 *	as itself is terminated. If ID > 0 and present, that
 *	process is terminated. 
 *
 * 	All result in the process being removed from the ready
 *	Queue. If the currently running process is terminated, 
 *	either a new PCB is switched to, or the OS IDLEs
 * 
 */
void terminate_Process ( INT32 process_ID, INT32 *error ){
	PCB_t * switchPCB;

	INT32	status;
	if (total_pid <= 0) ZCALL( Z502_HALT() );

	//if pid is -1; terminate self
	if ( process_ID == -1 ){

		CALL( status = rm_from_readyQueue(current_PCB->p_id) );
		if (status) (*error) = ERR_SUCCESS;
		else {
			(*error) = ERR_BAD_PARAM;
			debugPrint("TERMINATE PROCESS ERROR: ID DOES NOT EXIST");
		}

		//STATE PRINT OUT FOR TERMINATE
		if (TERMINATEpo && DEBUGFLAG){
			SP_setup( SP_TARGET_MODE, current_PCB->p_id );
			SP_setup( SP_TERMINATED_MODE, current_PCB->p_id );
			SP_print_header();
        	SP_print_line();
        	printReady();
		}
		//If there are ready pids, switch or IDLE
		if (total_pid > 0){
			CALL( switchPCB = get_readyPCB() );
			if (switchPCB == NULL) ZCALL( EVENT_IDLE() );
			if (switchPCB !=NULL) CALL( switch_Killcontext(switchPCB) );
		}
		//No more PIDS, HALT
		else CALL ( Z502_HALT() );
	}
	//if pid -2; terminate self and any child
	else if ( process_ID == -2 ){
		CALL( rm_children(&pidList, current_PCB->p_id) );

		CALL( status = rm_from_readyQueue(current_PCB->p_id) );
		if (status) (*error) = ERR_SUCCESS;
		else{
			debugPrint("TERMINATE PROCESS ERROR: ID DOES NOT EXIST");
			(*error) = ERR_BAD_PARAM;
		}

		//STATE PRINT OUT FOR TERMINATE
		if (TERMINATEpo && DEBUGFLAG){
			SP_setup( SP_TARGET_MODE, current_PCB->p_id );
			SP_setup( SP_TERMINATED_MODE, current_PCB->p_id );
			SP_print_header();
        	SP_print_line();
        	printReady();
		}

		if (total_pid > 0){
			CALL( switchPCB = get_readyPCB() );
			if (switchPCB == NULL) ZCALL( EVENT_IDLE() );
			if (switchPCB != NULL) CALL( switch_Killcontext(switchPCB) );
		}
		else ZCALL( Z502_HALT() );
	}
	//remove pid from pidList
	else{
		CALL( status = rm_from_readyQueue(process_ID) );
		if (status)	(*error) = ERR_SUCCESS;
		else{
			debugPrint("TERMINATE PROCESS ERROR: ID DOES NOT EXIST");
			(*error) = ERR_BAD_PARAM;
		}
		//STATE PRINT OUT FOR TERMINATE
		if (TERMINATEpo && DEBUGFLAG){
			SP_setup( SP_TARGET_MODE, process_ID );
			SP_setup( SP_TERMINATED_MODE, process_ID );
			SP_print_header();
        	SP_print_line();
        	printReady();
		}


		if (total_pid <= 0){
			ZCALL ( Z502_HALT() );
		}
	}
}
//	Helper function for terminate process
//	Cycles through ready list and removes all PCB's
//	whose parent ID matches the currently running PCB
void rm_children ( PCB_t ** ptrFirst, INT32 process_ID ){
	PCB_t * ptrCheck = *ptrFirst;

	SP_print_header();
	while (ptrCheck != NULL){
		if ( ptrCheck->p_parent == process_ID ){
			CALL( rm_from_readyQueue( ptrCheck->p_id ) );

			//PRINT OUT FOR TERMINATE 
			if (TERMINATEpo && DEBUGFLAG){
				SP_setup( SP_TARGET_MODE, current_PCB->p_id );
				SP_setup( SP_TERMINATED_MODE, ptrCheck->p_id );
        		SP_print_line();
			}
		}
		ptrCheck = ptrCheck->next;
	}
}

/*
 * SUSPEND PROCESS
 *
 *	This routine takes the process ID to be suspended from
 *	the OS. If we are suspending the current process, then
 * 	switch context or IDLE is called depending on any PCB's
 *	being ready to run. This routine passes success or fail
 *	to the OS, but returns no other values when called.
 *
 */
void suspend_Process ( INT32 process_ID, INT32 *error ){
	PCB_t * switchPCB;
	INT32	status;

	INT32 check;
	CALL( check = check_pid_ID(process_ID) );
	if (process_ID == -1) check = 1;
	if (check == -1){
		debugPrint("SUSPEND PROCESS ERROR: ID DOES NOT EXIST");
		(*error) = ERR_BAD_PARAM;
		return;
	}

	//if pid -1, suspend self
	if ( process_ID == -1 ){
		//RUNNING TO SUSPEND STATE
		CALL( status = ready_to_Suspend(current_PCB->p_id) );
		if (status == 1) (*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;

		//STATE PRINTOUT
		if (SUSPENDpo && DEBUGFLAG){
			SP_setup( SP_TARGET_MODE, current_PCB->p_id );
			SP_setup( SP_SUSPENDED_MODE, current_PCB->p_id );
			SP_print_header();
        	SP_print_line();
		}

		//GET READY PCB TO RUN OR IDLE
		CALL( switchPCB = get_readyPCB() );
		if (switchPCB == NULL) ZCALL( EVENT_IDLE() );
		if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
	}
	else{
		//READY TO SUSPEND STATE
		CALL( status = ready_to_Suspend(process_ID) );
		if (status == 1) (*error) = ERR_SUCCESS;
		else (*error) = ERR_BAD_PARAM;

		//STATE PRINTOUT
		if (SUSPENDpo && DEBUGFLAG){
			SP_setup( SP_TARGET_MODE, process_ID );
			SP_setup( SP_SUSPENDED_MODE, process_ID );
			SP_print_header();
        	SP_print_line();
        	printReady();
		}
	}
}

//Helper function to update PCB state
INT32 ready_to_Suspend ( INT32 process_ID ){
	ZCALL( lockReady() );
	PCB_t *ptrCheck = pidList;

	while ( ptrCheck != NULL ){
		if (ptrCheck->p_id == process_ID){
			if ( ptrCheck->p_state == SUSPENDED_STATE){
				ZCALL( unlockReady() );
				return -1;
			}
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
 *
 *	This function takes in the process ID to be resumed from
 *	suspension. It passes success or fail to the OS, but 
 *	returns no other values when called.
 *
 */
void resume_Process ( INT32 process_ID, INT32 *error ){
	ZCALL( lockReady() );
	PCB_t *ptrCheck = pidList;

	INT32 check;
	//Check that PCB exists
	CALL( check = check_pid_ID(process_ID) );
	if (process_ID == -1) check = 1;
	//Does not exist
	if (check == -1){
		debugPrint("RESUME PROCESS ERROR: ID DOES NOT EXIST");
		ZCALL( unlockReady() );
		(*error) = ERR_BAD_PARAM;
		return;
	}
	//Exists, check if the process is suspended
	while ( ptrCheck != NULL ){
		if (ptrCheck->p_id == process_ID){
			//If Suspended, update to Ready
			if (ptrCheck->p_state == SUSPENDED_STATE){
				ptrCheck->p_state = READY_STATE;
				ZCALL( unlockReady() );
				(*error) = ERR_SUCCESS;

				//STATE PRINTOUT
				if (RESUMEpo && DEBUGFLAG){
					SP_setup( SP_TARGET_MODE, ptrCheck->p_id );
					SP_setup( SP_RESUME_MODE, ptrCheck->p_id );
					SP_print_header();
	        		SP_print_line();
	        		printReady();
				}
				return;
			}
		}
		ptrCheck = ptrCheck->next;
	}
	//The process was not already suspended, return with bad param
	ZCALL( unlockReady() );
	(*error) = ERR_BAD_PARAM;
	return;
}
