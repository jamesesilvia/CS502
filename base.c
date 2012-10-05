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
extern BOOL          POP_THE_STACK;
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
    INT32              Status;
    INT32              Index = 0;
    INT32 			   currentTime;
    INT32 			   success;
    PCB_t 			   *switchPCB;
	INT32			   wokenUp;
	INT32			   sleeptime;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &Status );
	
	if (Status == ERR_BAD_PARAM){
		ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );
		return;
	}
	else{
		switch(device_id){
			case(TIMER_INTERRUPT):
//				printf("\nTIMER INTERRUPT OCCURED\n");
				CALL( currentTime = get_currentTime() );
				//Wake up all WAITING items that are before currentTime
				CALL( wokenUp = wake_timerList(currentTime) );
				//Get sleeptime
				CALL( sleeptime = checkTimer (currentTime) );
//				CALL( printf("SLEEPTIMEEEEEEE %d\n", sleeptime) );

				//New processes are Ready
				if( sleeptime > 0){
					if (wokenUp > 0){
						CALL( switchPCB = get_readyPCB() );
						if ( switchPCB->p_id == current_PCB->p_id ){
							CALL( Start_Timer(sleeptime) );
							ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );
						}
						else{
							CALL( Start_Timer(sleeptime) );
							ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );
							CALL( switch_Savecontext( switchPCB ) );
						}
					}
					//No new processes wokenUp
					else{
						CALL( Start_Timer(sleeptime) );
						ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );
					}
					break;
				}
				else{
					ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );
					break;
				}	
		}
		return;
    }
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
    void				*procPTR;
    
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
	else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1d" ) == 0 ) )
    	procPTR = test1d;
	else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1e" ) == 0 ) )
	    procPTR = test1e;
    else
    	procPTR = test1c;
	
    CALL( OS_Create_Process(CALLING_ARGV[1], procPTR, 0, &i, &i, 1) );
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
	PCB->next = NULL;
	PCB->prev = NULL;
	
	if (current_PCB != NULL) PCB->p_parent = current_PCB->p_id;
	
	//Add to Main List
	CALL( add_to_readyQueue(&pidList, PCB) );
//	CALL( priority_sort(&pidList) );
//	CALL( print_queues(&pidList) );

	(*error) = ERR_SUCCESS;
	(*pid) = PCB->p_id;
	
	if (SWITCH == 1) CALL( make_switch_Savecontext(PCB, procPTR) );
	if (SWITCH != 1) CALL( make_context(PCB, procPTR) );

	return 0; 
											/* End off OS_Create_Process */
}

/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
************************************************************************/

void    svc( void ) {
    INT16               call_type;
    static INT16        do_print = 0;
    INT32				Time;
    INT32 				sleepTime;
    INT32				currentTime;
    PCB_t				*temp;
	PCB_t				*switchPCB;

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
    		CALL( sleepTime = Z502_ARG1.VAL );
    		CALL( currentTime = get_currentTime() );
    		CALL( current_PCB->p_time = ( sleepTime + currentTime ) );

    		CALL( readyQueue_to_timerQueue( current_PCB->p_id  ) );

    		//Sleep
    		CALL( sleepTime = checkTimer(currentTime) );
    		CALL( Start_Timer( sleepTime ) );
			
			CALL( switchPCB = get_readyPCB() );
			if (switchPCB == NULL) ZCALL( Z502_IDLE() );
    		if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
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
    	//Suspend Process
    	case SYSNUM_SUSPEND_PROCESS:
    		CALL( suspend_Process((INT32)Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR) );
    		break;
    	case SYSNUM_RESUME_PROCESS:
    		CALL( resume_Process((INT32)Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR) );
    		break;
		default:
    		printf("ERROR! call_type not recognized!\n");
    		printf("Entered Call_Type is %i\n", call_type);
    		break;
    }											// End of switch call_type
}                                               // End of svc

void Start_Timer(INT32 Time) {	
	INT32		Status;
//	ZCALL ( HW_lock() );
	ZCALL( MEM_WRITE( Z502TimerStart, &Time ) );
	ZCALL( MEM_READ( Z502TimerStatus, &Status ) );
//	ZCALL ( HW_unlock() );
}

INT32 get_currentTime() {
	INT32 	currentTime;
//	ZCALL ( HW_lock() );
	ZCALL( MEM_READ( Z502ClockStatus, &currentTime ) );
//	ZCALL ( HW_unlock() );
	return currentTime;
}

