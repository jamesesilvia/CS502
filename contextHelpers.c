#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

// Added 9/4/2012
#include			"userdefs.h"

void make_context ( PCB_t * PCB, void *procPTR ){
	ZCALL( Z502_MAKE_CONTEXT( &PCB->context, procPTR, USER_MODE ));
}
void make_switch_Savecontext ( PCB_t * PCB, void *procPTR ){
	current_PCB = PCB;

	if( CREATEpo && DEBUGFLAG){
        SP_setup( SP_SWAPPED_MODE, inc_pid );
        SP_setup( SP_TARGET_MODE, current_PCB->p_id );	
        SP_print_header();
        SP_print_line();
    }

	ready_to_Running();
	ZCALL( Z502_MAKE_CONTEXT( &PCB->context, procPTR, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_SAVE_MODE, &PCB->context ));
}
void make_switch_Killcontext ( PCB_t * PCB, void *procPTR ){
	current_PCB = PCB;

	if( CREATEpo && DEBUGFLAG){
        SP_setup( SP_SWAPPED_MODE, inc_pid );
        SP_setup( SP_TARGET_MODE, current_PCB->p_id );
        SP_print_header();
        SP_print_line();
    }

	ready_to_Running();
	ZCALL( Z502_MAKE_CONTEXT( &PCB->context, procPTR, USER_MODE ));
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB->context ));
}
void switch_Killcontext ( PCB_t * PCB ){
	current_PCB = PCB;

	if( CREATEpo && DEBUGFLAG){
        SP_setup( SP_SWAPPED_MODE, inc_pid );
        SP_setup( SP_TARGET_MODE, current_PCB->p_id );
        SP_print_header();
        SP_print_line();
    }

	ready_to_Running();
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &PCB->context ));
}
void switch_Savecontext ( PCB_t * PCB ){
	current_PCB = PCB;

	if( CREATEpo && DEBUGFLAG){
        SP_setup( SP_SWAPPED_MODE, inc_pid );
        SP_setup( SP_TARGET_MODE, current_PCB->p_id );
        SP_print_header();
        SP_print_line();
    }
//	printf("CURRENT PCB %s\n",current_PCB->p_name);
	ready_to_Running();
	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_SAVE_MODE, &PCB->context ));
}

