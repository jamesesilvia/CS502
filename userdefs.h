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
#define			MAX_PIDs				100
#define			MAX_NAME				16

#define			DEBUGFLAG 				1

typedef         struct {
	char					p_name[MAX_NAME+1];
	INT32					p_id;
	INT32					p_state;
	INT32					p_priority;
	INT32					p_parent;
	INT32					p_time;
	void					*context;
	void					*next;
	void					*prev;
    }PCB_t;

INT32	OS_Create_Process( char *name, void *procPTR, INT32 priority, INT32 *pid, INT32 *error, INT32 SWITCH);
void	Start_Timer( INT32 Time );
void 	make_context( PCB_t * PCB, void *procPTR);
void 	make_switch_Savecontext( PCB_t * PCB, void *procPTR);
void 	make_switch_Killcontext( PCB_t * PCB, void *procPTR);
void 	switch_Killcontext( PCB_t * PCB );
void 	switch_Savecontext ( PCB_t * PCB );
void 	add_to_readyQueue ( PCB_t **ptrFirst, PCB_t *entry );
void 	add_to_timerQueue ( PCB_t **ptrFirst, PCB_t *entry );
INT32 	add_to_Queue( PCB_t **ptrFirst, PCB_t * entry);
INT32 	rm_from_readyQueue ( INT32 remove_id );
void 	rm_from_timerQueue ( PCB_t **ptrFirst, INT32 remove_id );
PCB_t 	*rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id );
void 	priority_sort( PCB_t ** ptrFirst );
void 	timer_sort( PCB_t ** ptrFirst );
INT32 	pid_Bounce( PCB_t **ptrFirst, INT32 id_check);
INT32 	check_name ( PCB_t **ptrFirst, char *name );
INT32 	get_PCB_ID( PCB_t **ptrFirst, char *name, INT32 *process_ID, INT32 *error );
INT32 	get_first_ID ( PCB_t ** ptrFirst );
PCB_t 	*get_firstPCB(PCB_t ** ptrFirst);
void 	terminate_Process( INT32 process_ID, INT32 *error );
void 	rm_children ( PCB_t **ptrFirst, INT32 process_ID );
void 	print_queues ( PCB_t **ptrFirst );
INT32 	get_currentTime();

extern 		PCB_t 			*current_PCB;
extern		PCB_t			*pidList;
extern		PCB_t			*timerList;
extern 		INT32 			inc_pid;
extern		INT32			total_pid;
