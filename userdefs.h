#include        "stdio.h"

/* Process Control Block Definitions */
// State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			HALTED_STATE			94

void	OS_Create_Process( void );

typedef         struct {
	INT32					p_id;
	INT32					p_state;
	INT32					*p_counter;
	struct task_struct 		*parent;
	INT32 					children;
	void					*next_context;

    }PCB_t;
