/*
*	This file contains all routines that handle paging
*
*	read_Disk()
*	write_Disk()
*
*/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include		"userdefs.h"

#define			MAXPGSIZE			1024

INT32 get_emptyFrame( INT32 pageRequest ){
	FRAMETABLE_t *ptrCheck = pageList;

	while( ptrCheck != NULL ){
		if(ptrCheck->page == -1){
			ptrCheck->p_id = current_PCB->p_id;
			ptrCheck->page = pageRequest;
			ptrCheck->refTime = get_currentTime();
			printf("FRAME %d\n", ptrCheck->frame);
			return ptrCheck->frame;
		}
		ptrCheck = ptrCheck->next;
	}
	//Table is Full
	return -1;
}

void check_pageSize( INT32 pageSize ){
	INT32	Index = 0;

	if (pageSize >= 0 && pageSize <= MAXPGSIZE-1){
		return;
	}
	else{
		debugPrint("FATAL ERROR IN PAGE SIZE");
		debugPrint("TERMINATING PROCESS AND CHILDREN");
		CALL( terminate_Process(-2, &Index) );
	}
}

void read_Disk( INT32 disk_id, INT32 sector, char *DATA ){
	INT32 	Temp, Temp1;

	ZCALL( lockDisks() );
	//ID, Sector, and Data
	ZCALL( MEM_WRITE( Z502DiskSetID, &disk_id ) );
    ZCALL( MEM_WRITE( Z502DiskSetSector, &sector ) );
    ZCALL( MEM_WRITE( Z502DiskSetBuffer, (INT32 *)DATA ) );
    //Specify a read
    Temp = 0;
    ZCALL( MEM_WRITE( Z502DiskSetAction, &Temp ) );
    //Start disk
    Temp = 0;                        // Must be set to 0
    ZCALL( MEM_WRITE( Z502DiskStart, &Temp ) );
    ZCALL( unlockDisks() );

    current_PCB->disk = disk_id;
    CALL( suspend_Process(-1, &Temp1) );
}

void write_Disk( INT32 disk_id, INT32 sector, char *DATA ){
	INT32	Temp, Temp1;

	ZCALL( lockDisks() );
	//ID, Sector, and Data
	ZCALL( MEM_WRITE( Z502DiskSetID, &disk_id ) );
    ZCALL( MEM_WRITE( Z502DiskSetSector, &sector ) );
    ZCALL( MEM_WRITE( Z502DiskSetBuffer, (INT32 *)DATA ) );
    //Specify a write
    Temp = 1;
    ZCALL( MEM_WRITE( Z502DiskSetAction, &Temp ) );
    //Start disk
    Temp = 0;                        // Must be set to 0
    ZCALL( MEM_WRITE( Z502DiskStart, &Temp ) );

    //Did it start? 
    //** CURRENTLY GETTING ERRONEOUS RESULT
    ZCALL( MEM_READ( Z502DiskStatus, &Temp ) );
    printf("DISK STATUS %d\n\n", Temp);
    if ( Temp == DEVICE_IN_USE )        // Disk should report being used
        printf( "Got expected result for Disk Status\n" );
    else
        printf( "Got erroneous result for Disk Status\n" );
	ZCALL( unlockDisks() );

    current_PCB->disk = disk_id;    
    CALL( suspend_Process( -1, &Temp1) );
}

void diskHandler( INT32 diskStatus ){

	switch(diskStatus){
		case(ERR_SUCCESS):
			CALL( wakeup_Disks() );
			printf("ERR_SUCCESS\n");
			break;
		case(ERR_BAD_PARAM):
			printf("ERR_BAD_PARAM\n");
			break;
		case(ERR_NO_PREVIOUS_WRITE):
			printf("ERR_NO_PREVIOUS_WRITE\n");
			break;
		case(ERR_DISK_IN_USE):
			printf("ERR_DISK_IN_USE\n");
			break;
	}
}

void wakeup_Disks( void ){
	PCB_t 	*ptrCheck = pidList;
	PCB_t 	*switchPCB;
	INT32	Temp = 0;

	while( ptrCheck != NULL ){
		if ( (ptrCheck->disk != -1) && 
			(ptrCheck->p_state == SUSPENDED_STATE) ){

			ptrCheck->disk = -1;
			CALL( resume_Process( ptrCheck->p_id, &Temp ) );

			CALL( switchPCB = get_readyPCB() );
            //No ready items, IDLE
            if (switchPCB == NULL){
            	ZCALL( EVENT_IDLE() );
            }
            //Switch if current PCB is not ready PCB
             else if ( switchPCB->p_id != current_PCB->p_id ){
             	CALL( switch_Savecontext( switchPCB ) );
             }
             //Return if they are the same
             else if ( switchPCB->p_id == current_PCB->p_id ){
             	return;
             }
             //Otherwise, IDLE
              else{
              	ZCALL( EVENT_IDLE() );
              }
		}
		ptrCheck = ptrCheck->next;
	}
}
