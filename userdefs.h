#include        "stdio.h"
#include		"stdlib.h"

/* Process Control Block Definitions */
// State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			SUSPENDED_STATE			94
//Not currently Used
#define			HALTED_STATE			95

#define			MAX_PIDs				100
#define			MAX_NAME				16
#define			MAX_MSG					128
#define			MAX_MSG_COUNT			8

#define			DEBUGFLAG 				1

typedef			struct{
	INT32					dest_ID;
	char 					message[MAX_MSG+1];
	void					*next;
	} MSG_t;

typedef         struct {
	char					p_name[MAX_NAME+1];
	INT32					p_id;
	INT32					p_state;
	INT32					p_priority;
	INT32					p_parent;
	INT32					p_time;
	INT32					msg_state;
	INT32					msg_count;
	void					*context;
	void					*next;
	void					*prev;
	void					*Inbox;
	void					*Outbox;
    } PCB_t;
	
typedef			struct {
	INT32					device_ID;
	INT32					Status;
	void					*next;
	} EVENT_t;


//Create Process
INT32	OS_Create_Process( char *name, void *procPTR, 
	INT32 priority, INT32 *pid, INT32 *error, INT32 SWITCH);
//Contexts
void 	make_context( PCB_t * PCB, void *procPTR);
void 	make_switch_Savecontext( PCB_t * PCB, void *procPTR);
void 	make_switch_Killcontext( PCB_t * PCB, void *procPTR);
void 	switch_Killcontext( PCB_t * PCB );
void 	switch_Savecontext ( PCB_t * PCB );
//Add Queues
void 	add_to_readyQueue ( PCB_t **ptrFirst, PCB_t *entry );
void 	add_to_timerQueue ( PCB_t **ptrFirst, PCB_t *entry );
void 	add_to_eventQueue ( INT32 *device_id, INT32 *status );
INT32 	add_to_msgQueue ( MSG_t *entry );
INT32 	add_to_Queue( PCB_t **ptrFirst, PCB_t * entry);
//Remove Queues
INT32 	rm_from_readyQueue ( INT32 remove_id );
void 	rm_from_timerQueue ( PCB_t **ptrFirst, INT32 remove_id );
void 	rm_from_eventQueue ( INT32 remove_id );
PCB_t 	*rm_from_Queue( PCB_t **ptrFirst, INT32 remove_id );
//Move Queues
void 	timerQueue_to_readyQueue( INT32 remove_id );
void 	readyQueue_to_timerQueue( INT32 remove_id );
//PCB Handle
void 	change_Priority( INT32 process_ID, INT32 new_priority, INT32 *error );
INT32 	updatePriority ( INT32 process_ID, INT32 new_priority );
void 	wait_to_Ready ( INT32 remove_id );
void	ready_to_Running ( void );
PCB_t 	*ready_to_Wait ( INT32 remove_id );
//Sort Queues
void 	timer_sort( void );
void 	timerSwap( PCB_t *Prev, PCB_t *Curr );
void 	ready_sort( void );
void 	readySwap( PCB_t *Prev, PCB_t *Curr );
//Queue Helpers
INT32 	check_name ( PCB_t **ptrFirst, char *name );
INT32 	get_PCB_ID( PCB_t **ptrFirst, char *name, INT32 *process_ID, INT32 *error );
INT32	check_pid_ID ( INT32 check_ID );
//Get PCB
PCB_t 	*get_firstPCB(PCB_t ** ptrFirst);
PCB_t 	*get_readyPCB( void );
//Process Handle
void 	terminate_Process( INT32 process_ID, INT32 *error );
void 	rm_children ( PCB_t **ptrFirst, INT32 process_ID );
void 	suspend_Process ( INT32 process_ID, INT32 *error );
void 	resume_Process ( INT32 process_ID, INT32 *error );
//Timer Functions
INT32 	get_currentTime( void );
INT32 	checkTimer ( INT32 currentTime );
INT32 	wake_timerList ( INT32 currentTime );
void	Start_Timer( INT32 Time );
//Debug
void 	printTimer ( void );
void	printReady ( void );
void	printEvent ( void );
//Idle
void 	EVENT_IDLE ( void );
//Handle Events
void 	eventHandler ( void );
//Send Message
void send_Message ( INT32 dest_ID, char *message, INT32 msg_Len, INT32 *error );
//Receive Message
void receive_Message ( INT32 src_ID, char *message, 
	INT32 msg_rcvLen, INT32 *msg_sndLen, INT32 *sender_ID, INT32 *error);

//Global variables
extern 		PCB_t 			*current_PCB;
extern		PCB_t			*pidList;
extern		PCB_t			*timerList;
extern		EVENT_t			*eventList;
extern 		INT32 			inc_pid;
extern		INT32			total_pid;
//extern		volatile 		INT32			ISR;
extern		INT32			event_count;
