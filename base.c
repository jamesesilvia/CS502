/************************************************************************

        This code forms the base of the operating system you will
        build.  It has only the barest rudiments of what you will
        eventually construct; yet it contains the interfaces that
        allow test.c and z502.c to be successfully built together.      

        Revision History:       
        1.0 August 1990
        1.1 December 1990: Portability attempted.
        1.3 July     1992: More Portability enhancements.
                           Add call to sample_code.
        1.4 December 1992: Limit (temporarily) printout in
                           interrupt handler.  More portability.
        2.0 January  2000: A number of small changes.
        2.1 May      2001: Bug fixes and clear STAT_VECTOR
        2.2 July     2002: Make code appropriate for undergrads.
                           Default program start is in test0.
        3.0 August   2004: Modified to support memory mapped IO
        3.1 August   2004: hardware interrupt runs on separate thread
        3.11 August  2004: Support for OS level locking
************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

// Added 9/4/2012
#include			"userdefs.h"

extern char          MEMORY[];  
//extern BOOL          POP_THE_STACK;
extern UINT16        *Z502_PAGE_TBL_ADDR;
extern INT16         Z502_PAGE_TBL_LENGTH;
extern INT16         Z502_PROGRAM_COUNTER;
extern INT16         Z502_INTERRUPT_MASK;
extern INT32         SYS_CALL_CALL_TYPE;
extern INT16         Z502_MODE;
extern Z502_ARG      Z502_ARG1;
extern Z502_ARG      Z502_ARG2;
extern Z502_ARG      Z502_ARG3;
extern Z502_ARG      Z502_ARG4;
extern Z502_ARG      Z502_ARG5;
extern Z502_ARG      Z502_ARG6;

extern void          *TO_VECTOR [];
extern INT32         CALLING_ARGC;
extern char          **CALLING_ARGV;

PCB_t		     	*pidList = NULL;
PCB_t		     	*timerList = NULL;
PCB_t		     	*readyList = NULL;
PCB_t				*current_PCB = NULL;

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ", 
                            "get_pid  ", "create   ", "term_proc", 
                            "suspend  ", "resume   ", "ch_prior ", 
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };

INT32	 			inc_pid = 0;
INT32				total_pid = 0;

volatile 			timer_lock = 0;
volatile 			ready_lock = 0;

/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/
void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;
    INT32 			   currenttime;
    INT32 			   success;
    PCB_t 			   *Check;
    INT32 			   checkTime;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );



    switch(device_id){
    	case(TIMER):
			printf("\nTIMER INTERRUPT OCCURED\n");
    		CALL( Check = get_firstPCB(&timerList) );
    		CALL( checkTime = get_currentTime() );
    		if (Check == NULL) return;

    		while( Check->p_time >= checkTime ){
    			printf("MADE IT");
    			CALL( lockTimer() );
    			CALL( lockReady() );
    			CALL( add_to_Queue(&pidList, Check, PID_Q) );

    			CALL( success = rm_from_Queue(&timerList, Check->p_id, TIMER_Q ) );
    			if ( success ) printf("Successfully removed ID");
    			CALL( unlockTimer() );

    			CALL( priority_sort(&pidList) );
    			CALL( unlockReady() );

    			CALL( lockTimer() );
    			CALL( Check = get_firstPCB(&timerList) );
    			CALL( unlockTimer() );

    			CALL( print_queues(&timerList) );

    			if (Check == NULL) break;
    		}


//    		CALL( lockTimer() );
//    		CALL( lockReady() );
//    		CALL( add_to_Queue(&pidList, get_firstPCB(&timerList), PID_Q) );
//    		CALL( success = rm_from_Queue(&timerList, timerList->p_id, TIMER_Q ) );
//    		CALL( unlockTimer() );
//    		if ( success ) printf("Successfully removed ID");
//
//    		CALL( priority_sort(&pidList) );
//    		CALL( unlockReady() );

    		CALL( switch_save_context(get_firstPCB(&pidList)) );
    		ZCALL( MEM_READ( Z502ClockStatus, &currenttime ) );
    		break;
    }

    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of interrupt_handler */
/************************************************************************
    FAULT_HANDLER
        The beginning of the OS502.  Used to receive hardware faults.
************************************************************************/

void    fault_handler( void )
    {
    INT32       device_id;
    INT32       status;
    INT32       Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    printf( "Fault_handler: Found vector type %d with value %d\n",
                        device_id, status );

    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of fault_handler */


/************************************************************************
    OS_SWITCH_CONTEXT_COMPLETE
        The hardware, after completing a process switch, calls this routine
        to see if the OS wants to do anything before starting the user
        process.
************************************************************************/

void    os_switch_context_complete( void )
    {
    static INT16        do_print = TRUE;

    if ( do_print == TRUE )
    {
        printf( "os_switch_context_complete  called before user code.\n");
        do_print = FALSE;
    }
}                               /* End of os_switch_context_complete */

/************************************************************************
    OS_INIT
        This is the first routine called after the simulation begins.  This
        is equivalent to boot code.  All the initial OS components can be
        defined and initialized here.
************************************************************************/

void    os_init( void )
    {
    void                *next_context;
    INT32               i;
    void		*procPTR;
    
   /* Demonstrates how calling arguments are passed thru to here       */

    printf( "Program called with %d arguments:", CALLING_ARGC );
    printf( "\n" );

    /*          Setup so handlers will come to code in base.c           */

    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;


    /*  Determine if the switch was set, and if so go to demo routine.  */

    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "sample" ) == 0 ) )
        {
        ZCALL( Z502_MAKE_CONTEXT( &next_context,
                                        (void *)sample_code, KERNEL_MODE ));
        ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &next_context ));
    }                   /* This routine should never return!!           */



    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test0" ) == 0 ) )
        procPTR = test0;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1a" ) == 0 ) )
        procPTR = test1a;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1b" ) == 0 ) )
        procPTR = test1b;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1c" ) == 0 ) )
    	procPTR = test1c;
    else
    	procPTR = test1c;
	
    OS_Create_Process(CALLING_ARGV[1], procPTR, 100, &i, &i, 1);
}                                               /* End of os_init       */

INT32	OS_Create_Process( char * name, void * procPTR,
		INT32 priority, INT32 *pid, INT32 *error, INT32 SWITCH){

	printf("\n\nTOTAL PIDs %d\n\n", total_pid);

	inc_pid++;

	if (priority < 0){
		printf("BAD PRIORITY");
		(*error) = ERR_BAD_PARAM;
		return -1;
	}
	if (pidList != NULL){
		if(check_name(&pidList, name) == 0){
			printf("Duplicate Name");
			(*error) = ERR_BAD_PARAM;
			return -1;
		}	
	}
	//MAX_PIDs = 100 in userdefs
	if (total_pid >= MAX_PIDs){
		printf("Exceeded max nubmer of PIDs\n");
		(*error) = ERR_BAD_PARAM;
		return -1;
	}	

    PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));
	PCB->p_state = NEW_STATE;
	PCB->p_id = inc_pid;

	PCB->p_time = 0;

	memset(PCB->p_name, 0, MAX_NAME+1);
	strcpy(PCB->p_name,name);
	PCB->p_priority = priority;
	
	if (current_PCB != NULL) PCB->p_parent = current_PCB->p_id;
	
	//Add to Main List
	CALL( lockReady() );
	CALL( add_to_Queue(&pidList, PCB, PID_Q ) );
	CALL( priority_sort(&pidList) );
	CALL( print_queues(&pidList) );
	CALL( unlockReady() );

	(*error) = ERR_SUCCESS;
	(*pid) = PCB->p_id;
	
	if (SWITCH == 1) CALL( make_switch_context(PCB, procPTR) );
	if (SWITCH != 1) CALL( make_context(PCB, procPTR) );

	return 0; 
											/* End off OS_Create_Process */
}

void make_switch_context ( PCB_t * PCB, void *procPTR ){
	current_PCB = PCB;
	ZCALL( Z502_MAKE_CONTEXT( &PCB->context, procPTR, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_SAVE_MODE, &PCB->context ));
}

void make_context ( PCB_t * PCB, void *procPTR ){
	ZCALL( Z502_MAKE_CONTEXT( &PCB->context, procPTR, USER_MODE ));
}

void switch_context ( PCB_t * PCB ){
	current_PCB = PCB;
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB->context ));
}

void switch_save_context ( PCB_t * PCB ){
	current_PCB = PCB;
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_SAVE_MODE, &PCB->context ));
}

/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
************************************************************************/

void    svc( void ) {
    INT16               call_type;
    static INT16        do_print = 10;
    INT32				Time;
    INT32 				sleepTime;

    call_type = (INT16)SYS_CALL_CALL_TYPE;
    if ( do_print > 0 ) {
        printf( "SVC handler: %s %8ld %8ld %8ld %8ld %8ld %8ld\n",
                call_names[call_type], Z502_ARG1.VAL, Z502_ARG2.VAL, 
                Z502_ARG3.VAL, Z502_ARG4.VAL, 
                Z502_ARG5.VAL, Z502_ARG6.VAL );
        do_print--;
    }
    //Test0 from slides
    switch (call_type){
    	//Get time of day
    	case SYSNUM_GET_TIME_OF_DAY:
    		ZCALL(MEM_READ(Z502ClockStatus, &Time));
    		*(INT32 *)Z502_ARG1.PTR = Time;
    		break;
    	//Terminate a process
    	case SYSNUM_TERMINATE_PROCESS:
		printf("\n\nTerminate Process Called\n\n");
    		CALL( terminate_Process( (INT32)Z502_ARG1.VAL, (INT32*)Z502_ARG2.PTR ) );
    		break;
    	//Sleep
    	case SYSNUM_SLEEP:
    		sleepTime = Z502_ARG1.VAL;
    		current_PCB->p_time = (sleepTime + get_currentTime() );

    		CALL( lockTimer() );
    		CALL( add_to_Queue( &timerList, current_PCB, TIMER_Q ) );
//    		CALL( print_queues(&timerList) );
//    		CALL( timer_sort( &timerList ) );
    		CALL( unlockTimer() );
    		//Based on sorted Timer
    		//Start Timer on first p_time-current time
    		CALL( Start_Timer( timerList->p_time-get_currentTime() ) );
//   		CALL( Start_Timer(sleepTime) );

//    		CALL( switch_context(pidList) );
    		break;

    	//Create Process
    	case SYSNUM_CREATE_PROCESS:
    		CALL( OS_Create_Process((char*)Z502_ARG1.PTR, (void *)Z502_ARG2.PTR,
    				(INT32)Z502_ARG3.VAL,(INT32*)Z502_ARG4.PTR, (INT32*)Z502_ARG5.PTR, 0) );
    		break;
    	//Get Process ID
    	case SYSNUM_GET_PROCESS_ID:
    		CALL( get_PCB_ID(&pidList, (char *)Z502_ARG1.PTR,
    				(INT32 *)Z502_ARG2.PTR, (INT32 *)Z502_ARG3.PTR) );
    		break;
		default:
    		printf("ERROR! call_type not recognized!\n");
    		printf("Entered Call_Type is %i\n", call_type);
    		break;
    }											// End of switch call_type
}                                               // End of svc

INT32 add_to_Queue( PCB_t **ptrFirst, PCB_t * entry, INT32 listFlag ){

	//First Case
	if ( *ptrFirst == NULL){
		(*ptrFirst) = entry;
		if (listFlag == PID_Q) total_pid++;
		if (listFlag == TIMER_Q){
			printf("TIMERQQQ");
		}
		return 1;
	}

	//Add to start of list
	else{
		(*ptrFirst)->prev = entry;
		entry->next = (*ptrFirst);
		(*ptrFirst) = entry;
		//MAIN LIST
		if (listFlag == PID_Q) total_pid++;

		return 1;
	}
	return 0;
}

void print_queues ( PCB_t **ptrFirst ){
	if (*ptrFirst == NULL) return;
	PCB_t * ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		printf("------------------------------\n");
		printf("Name: %s\t", ptrCheck->p_name);
		printf("ID: %d\t", ptrCheck->p_id);
		printf("Time: %d\n", ptrCheck->p_time);
		ptrCheck = ptrCheck->next;
	}
	return;
}

INT32 rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id, INT32 listFlag ){
	PCB_t * ptrDel = *ptrFirst;
	PCB_t * ptrPrev = NULL;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){

			//First ID
			if ( ptrPrev == NULL){
				(*ptrFirst) = ptrDel->next;
				//MAIN LIST
				if (listFlag) total_pid--;
				//TIMER LIST
				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );
				return 1;
			}
			//Last ID
			else if (ptrDel->next == NULL){
				ptrPrev->next = NULL;
				if (listFlag) total_pid--;
				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );
				return 1;

			}
			//Middle ID
			else{
				PCB_t * ptrNext = ptrDel->next;
				ptrPrev->next = ptrDel->next;
				ptrNext->prev = ptrDel->prev;
				if (listFlag) total_pid--;
				if ( listFlag == TIMER_Q ) add_to_Queue( &pidList, ptrDel, PID_Q );
				return 1;
			}
		}
		ptrPrev = ptrDel;
		ptrDel = ptrDel->next;;
	}
	//No ID in PCB List
	return 0;
}

INT32 pid_Bounce( PCB_t **ptrFirst, INT32 id_check ) {
	PCB_t * ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
		if (ptrCheck->p_id == id_check){
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	return 1;
}

INT32 check_name( PCB_t **ptrFirst, char *name ){
	PCB_t *ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
		if(strcmp(ptrCheck->p_name, name) == 0){
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	return 1;
}

INT32 get_PCB_ID(PCB_t ** ptrFirst, char *name, INT32 *process_ID, INT32 *error){
	
	if (strcmp("", name) == 0){
		(*process_ID) = current_PCB->p_id;
		(*error) = ERR_SUCCESS;
//		printf("PROCESS ID: %d", current_PCB->p_id);
		return 0;
	}
	PCB_t *ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		if (strcmp(ptrCheck->p_name, name) == 0){
			(*process_ID) = ptrCheck->p_id;
			(*error) = ERR_SUCCESS;
//			printf("PROCESS ID: %d", ptrCheck->p_id);
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	(*error) = ERR_BAD_PARAM;
//	printf("ERROR BAD PARAM");
	return -1;
}

INT32 get_first_ID ( PCB_t ** ptrFirst ){
	PCB_t *ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
			ptrCheck = ptrCheck->next;
		}
	return ptrCheck->p_id;
}

PCB_t * get_firstPCB(PCB_t ** ptrFirst){
	PCB_t *ptrCheck = *ptrFirst;

	while (ptrCheck != NULL){
		ptrCheck = ptrCheck->next;
	}
	return ptrCheck;
}

void terminate_Process ( INT32 process_ID, INT32 *error ){

	//if pid is -1; terminate self
	if ( process_ID == -1 ){
		//printf("\nTerminate self\n");

		if (rm_from_Queue(&pidList, current_PCB->p_id, PID_Q))
			(*error) = ERR_SUCCESS;
		else 	(*error) = ERR_BAD_PARAM;

		if (total_pid > 0){
			switch_context(pidList);		
		}
		else CALL ( Z502_HALT() );
	}	
	//if pid -2; terminate self and any child
	else if ( process_ID == -2 ){
		//printf("\nTerminate self and children\n");
		rm_children(&pidList, current_PCB->p_id);
		
		if (rm_from_Queue(&pidList, current_PCB->p_id, PID_Q))
			(*error) = ERR_SUCCESS;
		else 	(*error) = ERR_BAD_PARAM;

		if (total_pid > 0){
			switch_context(pidList);		
		}
		else CALL ( Z502_HALT() );
	}
	//remove pid from pidList
	else{
		if (rm_from_Queue(&pidList, process_ID, PID_Q))
			(*error) = ERR_SUCCESS;
		else	(*error) = ERR_BAD_PARAM;

		if (total_pid <= 0){
			CALL ( Z502_HALT() );
		}
	}
}
//Helper function for terminate process
void rm_children ( PCB_t ** ptrFirst, INT32 process_ID ){
	PCB_t * ptrCheck = *ptrFirst;
	while (ptrCheck != NULL){
		if ( ptrCheck->p_parent == process_ID ){
			rm_from_Queue(&pidList, ptrCheck->p_id, PID_Q);
		}
		ptrCheck = ptrCheck->next;
	}
}

void priority_sort( PCB_t ** ptrFirst ){
	PCB_t * ptrCheck = *ptrFirst;
	PCB_t * temp = NULL;

	if ( ptrCheck->next == NULL ) return;

	PCB_t * ptrNext = ptrCheck->next;

	while ( ptrNext != NULL ){
		if ( ptrCheck->p_priority >= ptrNext->p_priority ){
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

void timer_sort( PCB_t ** ptrFirst ){
	PCB_t * ptrCheck = *ptrFirst;
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
		if ( ptrCheck->p_time >= ptrNext->p_time ){
			printf("STUCK%d %d",ptrCheck->p_id, ptrNext->p_id);

			temp = ptrCheck;
			ptrCheck->prev = ptrNext;
			ptrCheck->next = ptrNext->next;
			ptrNext->prev = temp->prev;
			ptrNext->next = temp;
		}
		if ( ptrCheck->next == NULL ) return;
		ptrNext = ptrCheck->next;
	}
	return;
}

void Start_Timer(INT32 Time) {
	INT32		Status;
	MEM_WRITE( Z502TimerStart, &Time );
	MEM_READ( Z502TimerStatus, &Status );
	ZCALL( Z502_IDLE() );
}

INT32 get_currentTime() {
	INT32 	currenttime;
	ZCALL( MEM_READ( Z502ClockStatus, &currenttime ) );
	return currenttime;
}

