
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

// Added 9/4/2012
#include			"userdefs.h"

extern void          *TO_VECTOR [];
extern INT32         CALLING_ARGC;
extern char          **CALLING_ARGV;

int 			inc_pid=0;
PCB_t			created_PCB;

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
	PCB_t			PCB;
	void 			*procPTR;

    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test0" ) == 0 ) )
        procPTR = test0;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1a" ) == 0 ) )
        procPTR = test1a;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1b" ) == 0 ) )
            procPTR = test1b;
    else
    	procPTR = test1a;

	inc_pid++;
	PCB.p_state = READY_STATE;
	PCB.p_id = inc_pid;

	created_PCB = PCB;

	ZCALL( Z502_MAKE_CONTEXT( &PCB.next_context, procPTR, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB.next_context ));
											/* End off OS_Create_Process */
}
