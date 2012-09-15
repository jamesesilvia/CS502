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
#define			COUNT			1
#define			NOCOUNT			0
#define			MAX_PIDs		100
#define			MAX_NAME		16

typedef         struct {
	char					p_name[MAX_NAME+1];
	INT32					p_id;
	INT32					p_state;
	INT32					p_priority;
	INT32					p_parent;
	void					*context;
	void					*next;
	void					*prev;

    }PCB_t;

INT32	OS_Create_Process( char *name, void *procPTR, INT32 priority, INT32 *pid, INT32 *error, INT32 SWITCH);
//void 	Add_to_TQueue( PCB_t *ptrFirst, PCB_t *entry );
void	Start_Timer( INT32 Time );
void make_switch_context( PCB_t * PCB, void *procPTR);
void make_context( PCB_t * PCB, void *procPTR);
void switch_context( PCB_t * PCB );
int add_to_Queue( PCB_t **ptrFirst, PCB_t * entry, INT32 toCount );
int rm_from_Queue( PCB_t **ptrFirst, int remove_id, INT32 toCount );
int pid_Bounce( PCB_t **ptrFirst, int id_check);
int check_name ( PCB_t **ptrFirst, char *name );
int get_PCB_ID( PCB_t **ptrFirst, char *name, INT32 *process_ID, INT32 *error );
void terminate_Process( INT32 process_ID, INT32 *error );
void rm_children ( PCB_t **ptrFirst, INT32 process_ID );
void print_queues ( PCB_t **ptrFirst );

extern 		PCB_t 			*created_PCB;
extern 		int 			inc_pid;
extern		int			total_pid;
