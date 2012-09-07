
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
int			current_pid;

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
    for ( i = 0; i < CALLING_ARGC; i++ )
        printf( " %s", CALLING_ARGV[i] );
    printf( "\n" );
    printf( "Calling with argument 'sample' executes the sample program.\n" );

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

    /*  This should be done by a "os_make_process" routine, so that
        test0 runs on a process recognized by the operating system. */
//    ZCALL( Z502_MAKE_CONTEXT( &next_context, (void *)test0, USER_MODE ));
//    ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &next_context ));

// 	9/4/12
    OS_Create_Process();

}                                               /* End of os_init       */

int	OS_Create_Process( void ){
	PCB_t			PCB;
	
	inc_pid++;
	current_pid = inc_pid;

	PCB.p_state = READY_STATE;
	PCB.p_id = inc_pid;

	//printf("\nOS_Create_Process called\n");
    //ZCALL( Z502_MAKE_CONTEXT( &PCB.next_context, (void *)test0, USER_MODE ));
	ZCALL( Z502_MAKE_CONTEXT( &PCB.next_context, (void *)test1a, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB.next_context ));

	return inc_pid;
											/* End off OS_Create_Process */
}
