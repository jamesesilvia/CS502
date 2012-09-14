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

PCB_t				*pidList = NULL;
PCB_t				*timerList = NULL;
PCB_t				*readyList = NULL;

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ", 
                            "get_pid  ", "create   ", "term_proc", 
                            "suspend  ", "resume   ", "ch_prior ", 
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };

int 			inc_pid=0;
PCB_t			*created_PCB;



/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/
void    interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;
    static BOOL        remove_this_in_your_code = TRUE;   /** TEMP **/
    static INT32       how_many_interrupt_entries = 0;    /** TEMP **/

    INT32 currenttime;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    // Start of Test1a
    switch(device_id){
    	case(TIMER):
			if ( rm_from_Queue(timerList, inc_pid) )
				printf("Successfully removed ID: %d", inc_pid);

    		printf("TIMER INTERRUPT OCCURED\n");
    		ZCALL( MEM_READ( Z502ClockStatus, &currenttime ) );

    		break;
    }


    /** REMOVE THE NEXT SIX LINES **/
    how_many_interrupt_entries++;                         /** TEMP **/
    if ( remove_this_in_your_code && ( how_many_interrupt_entries < 20 ) )
        {
        printf( "Interrupt_handler: Found device ID %d with status %d\n",
                        device_id, status );
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
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
************************************************************************/

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

    if (CALLING_ARGC > 1)
    	OS_Create_Process(CALLING_ARGV[1]);
    else
    	OS_Create_Process(NULL);

}                                               /* End of os_init       */

void	OS_Create_Process( char * proc ){
	void 			*procPTR;

    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test0" ) == 0 ) )
        procPTR = test0;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1a" ) == 0 ) )
        procPTR = test1a;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1b" ) == 0 ) )
        procPTR = test1b;
    else
    	procPTR = test1a;

    PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));

	inc_pid++;
	PCB->p_state = NEW_STATE;
	PCB->p_id = inc_pid;

	created_PCB = PCB;

	//Add to Main List
	add_to_Queue(pidList, created_PCB);

	ZCALL( Z502_MAKE_CONTEXT( &PCB->next_context, procPTR, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB->next_context ));
											/* End off OS_Create_Process */
}

void    svc( void ) {
    INT16               call_type;
    static INT16        do_print = 10;
    INT32				Time;

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
    	case SYSNUM_GET_TIME_OF_DAY:
    		ZCALL(MEM_READ(Z502ClockStatus, &Time));
    		*(INT32 *)Z502_ARG1.PTR = Time;
    		break;
    	case SYSNUM_TERMINATE_PROCESS:
    		Z502_HALT();
    		break;
    	//Added for Test1a
    	case SYSNUM_SLEEP:
    		add_to_Queue( timerList, created_PCB );
    		Start_Timer(Z502_ARG1.VAL);
    		break;
    	//Added for Test1b
    	case SYSNUM_CREATE_PROCESS:
    		printf("Remove; placeholder");
    		break;
    	default:
    		printf("ERROR! call_type not recognized!\n");
    		printf("Entered Call_Type is %i\n", call_type);
    		break;
    }											// End of switch call_type
    //End of Test0 code from slides
}                                               // End of svc

int add_to_Queue( PCB_t *ptrFirst, PCB_t * entry ){

	//ID and Name Check
	//To return 0

	//First Case
	if ( ptrFirst == NULL){
		ptrFirst = entry;
		return 1;
	}

	//Add to start of list
	else{
		ptrFirst->prev = entry;
		ptrFirst = entry;
		return 1;
	}
	return 0;
}

int rm_from_Queue( PCB_t *ptrFirst, int remove_id){
	PCB_t * ptrDel = ptrFirst;
	PCB_t * ptrPrev = NULL;
	PCB_t * ptrNext = NULL;

	while ( ptrDel != NULL ){
		if (ptrDel->p_id == remove_id){
			//First ID
			if ( ptrPrev == NULL){
				ptrFirst = ptrDel->next;
				ptrFirst->prev = NULL;
				printf("FOUND THE FIRST ONE");
				return 1;
			}
			//Last ID
			else if (ptrNext == NULL){
				ptrPrev->next = NULL;
				return 1;

			}
			else{
				ptrPrev->next = ptrDel->next;
				ptrNext->prev = ptrDel->prev;
				return 1;
			}
		}
		ptrPrev = ptrDel;
		ptrDel = ptrDel->next;
		ptrNext = ptrDel->next;
	}
	//No ID in PCB List
	return 0;
}

int pid_Bounce( PCB_t *ptrFirst, int id_check) {
	PCB_t * ptrCheck = ptrFirst;

	while (ptrCheck != NULL){
		if (ptrCheck->p_id == id_check){
			return 0;
		}
		ptrCheck = ptrCheck->next;
	}
	return 1;
}

/*void Add_to_TQueue( PCB_t *ptrFirst, PCB_t * entry ){
	//printf("\n\nSleeptime %ld\n\n", sleeptime);
	//printf("\n\nCurrent PID: %d", created_PCB.p_id);
	INT32 currenttime;

	created_PCB->p_state = RUNNING_STATE;
	ZCALL( MEM_READ( Z502ClockStatus, &currenttime ) );

	//created_PCB->p_counter = currenttime + sleeptime;
	//Start_Timer(sleeptime);

	if (ptrFirst == NULL){
			//ptrFirst = created_PCB;
			//ptrFirst->next = NULL;
			//printf("\n\n******%d\n\n", ptrFirst->p_counter);
			//printf("**");
	}
}*/

void Start_Timer(INT32 Time) {
	INT32		Status;
	MEM_WRITE( Z502TimerStart, &Time );
	MEM_READ( Z502TimerStatus, &Status );
	ZCALL( Z502_IDLE() );
} 

