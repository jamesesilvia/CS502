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
		terminate_Process(-2, &Index);
	}
}
/*	Not Currently Used
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
} */

void read_Disk( INT32 disk_id, INT32 sector, char *DATA ){
	INT32 	Temp;
	char	disk_buffer_read[PGSIZE];

	memset(disk_buffer_read, 0, 16);

	MEM_WRITE( Z502DiskSetID, &disk_id );
	MEM_READ( Z502DiskStatus, &Temp );

	while( Temp != DEVICE_FREE ){
        ZCALL( Z502_IDLE() );       
        MEM_READ( Z502DiskStatus, &Temp );
    }

    MEM_WRITE( Z502DiskSetSector, &sector );
    MEM_WRITE( Z502DiskSetBuffer, (INT32 *)disk_buffer_read );
    Temp = 0;                        // Specify a read
    MEM_WRITE( Z502DiskSetAction, &Temp );
    Temp = 0;                        // Must be set to 0
    MEM_WRITE( Z502DiskStart, &Temp );

    /* wait for the disk action to complete.  */
    MEM_WRITE( Z502DiskSetID, &disk_id );
    MEM_READ( Z502DiskStatus, &Temp );
    while( Temp != DEVICE_FREE )  {
        ZCALL( Z502_IDLE() );       
        MEM_READ( Z502DiskStatus, &Temp );
    }
    strcpy(DATA, disk_buffer_read);
}

void write_Disk( INT32 disk_id, INT32 sector, char *DATA ){
	INT32	Temp;
	char	disk_buffer_write[PGSIZE];

	strncpy( disk_buffer_write, DATA, 15 );

	MEM_WRITE( Z502DiskSetID, &disk_id );
    MEM_READ( Z502DiskStatus, &Temp );
    if ( Temp == DEVICE_FREE )        // Disk hasn't been used - should be free
        printf( "Got expected result for Disk Status\n" );
    else
        printf( "Got erroneous result for Disk Status - Device not free.\n" );

    MEM_WRITE( Z502DiskSetSector, &sector );
    MEM_WRITE( Z502DiskSetBuffer, (INT32 *)disk_buffer_write );
    Temp = 1;                        // Specify a write
    MEM_WRITE( Z502DiskSetAction, &Temp );
    Temp = 0;                        // Must be set to 0
    MEM_WRITE( Z502DiskStart, &Temp );
}
