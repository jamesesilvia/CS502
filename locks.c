/*
 * This file contains the functions used for spin locks
 * as well as all relevant global declarations
 *
 * Spin locks will be used on the ready queue, and timer queue
 */

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include             "userdefs.h"

extern volatile 	timer_lock;
extern volatile 	ready_lock;

void lockTimer( void );
void unlockTimer( void );
void lockReady( void );
void unlockReady ( void );


//Timer Locks
void lockTimer( void ){
	while(timer_lock) {}
	timer_lock = 1;
}
void unlockTimer ( void ){
	timer_lock = 0;
}

//Ready Locks
void lockReady ( void ){
	while(ready_lock) {}
	ready_lock = 1;
}
void unlockReady ( void ){
	ready_lock = 0;
}

