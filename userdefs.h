#include        "stdio.h"

/* Process Control Block Definitions */
// State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			HALTED_STATE			94

void	OS_Create_Process( char * proc );
void 	Add_to_Queue( INT32 sleeptime );
void	Start_Timer( void );

typedef         struct {
	INT32					p_id;
	INT32					p_state;
	INT32					p_counter;
	void					*next_context;

    }PCB_t;

extern 		PCB_t 			created_PCB;
extern 		int 			inc_pid;

