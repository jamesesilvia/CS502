#include        "stdio.h"
#include		"stdlib.h"

/* Process Control Block Definitions */
// State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			HALTED_STATE			94

#define			TIMER					4

typedef         struct {
	INT32					p_id;
	INT32					p_state;
	INT32					p_counter;
	void					*next_context;
	void					*next;
	void					*prev;

    }PCB_t;

void	OS_Create_Process( char * proc );
//void 	Add_to_TQueue( PCB_t *ptrFirst, PCB_t *entry );
void	Start_Timer( INT32 Time );
int add_to_Queue( PCB_t *ptrFirst, PCB_t * entry );
int rm_from_Queue( PCB_t *ptrFirst, int remove_id);
int pid_Bounce( PCB_t *ptrFirst, int id_check);

extern 		PCB_t 			*created_PCB;
extern 		int 			inc_pid;

