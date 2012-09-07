#include        "stdio.h"

/* Process Control Block Definitions */
// State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			HALTED_STATE			94

int	OS_Create_Process( void );
void	Start_Timer( void );

extern int inc_pid;
extern int current_pid;

typedef         struct {
	INT32					p_id;
	INT32					p_state;
	INT32					p_counter;
	void					*next_context;

    }PCB_t;
