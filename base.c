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

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ", 
                            "get_pid  ", "create   ", "term_proc", 
                            "suspend  ", "resume   ", "ch_prior ", 
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };


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

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id ); 
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    // Start of Test1a
    //switch(device_id):

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
		Start_Timer();
    		ZCALL( Z502_IDLE() );
    		break;
    	default:
    		printf("ERROR! call_type not recognized!\n");
    		printf("Entered Call_Type is %i\n", call_type);
    		break;
    }											// End of switch call_type
    //End of Test0 code from slides
}                                               // End of svc

void Start_Timer( void ) {
	INT32		Temp=100;
	INT32		Status;
	MEM_WRITE( Z502TimerStart, &Temp );
	MEM_READ( Z502TimerStatus, &Status );
} 

