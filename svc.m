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
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
************************************************************************/

void    svc( void ) {
    INT16               call_type;
    static INT16        do_print = 10;
    INT32				Time;
    INT32				Temp;
    INT32				Status;

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
    		Temp = 777;
    		MEM_WRITE( Z502TimerStart, &Temp );
    		MEM_READ( Z502TimerStatus, &Status );
    		ZCALL( Z502_IDLE() );
    		break;
    	default:
    		printf("ERROR! call_type not recognized!\n");
    		printf("Entered Call_Type is %i\n", call_type);
    		break;
    }											// End of switch call_type
    //End of Test0 code from slides
}
