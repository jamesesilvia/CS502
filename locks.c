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

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE

void lockTimer( void );
void unlockTimer( void );
void lockReady( void );
void unlockReady ( void );

//TimerQueue Locks
void lockTimer( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
void unlockTimer ( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
//ReadyQueue Locks
void lockReady ( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
void unlockReady ( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
//Both Locks
void lockBoth ( void ){
	lockTimer();
	lockReady();
}
void unlockBoth ( void ){
	unlockTimer();
	unlockReady();
}
//Z502 Timer Lock
void HW_lock( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
void HW_unlock ( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
//LOCK ALL
void LOCKALL( void ) {
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 2, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 3, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 4, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 5, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 6, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 7, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 8, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 9, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}
void UNLOCKALL( void ){
	INT32 LockResult;
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 1, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 2, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 3, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 4, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 5, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 6, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 7, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 8, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
	Z502_READ_MODIFY( MEMORY_INTERLOCK_BASE + 9, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult );
}


