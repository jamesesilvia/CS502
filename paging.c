/*
*	This file contains all routines that handle paging
*
*	MEM_READ
*	
*
*/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include		"userdefs.h"

#define			MAXPGSIZE			1023

void check_pageSize( INT32 pageSize ){
	INT32	Index = 0;

	if (pageSize > 0 && pageSize < MAXPGSIZE){
		return;
	}
	else{
		debugPrint("FATAL ERROR IN PAGE SIZE");
		debugPrint("TERMINATING PROCESS AND CHILDREN");
		terminate_Process(-2, &Index);
	}
}

INT32 manage_Table( INT32 ID, UINT16 frame){
	FRAMETABLE_t *ptrCheck;
	ptrCheck = pageList;
	
	while( ptrCheck != NULL ){
		if( ptrCheck->PID == -1){
			ptrCheck->PID = ID;
			ptrCheck->frame = frame;
			ptrCheck->refTime = get_currentTime();
			return 0;		
		}
		ptrCheck = ptrCheck->next;
	}
	//Table is full
	return -1;
}
