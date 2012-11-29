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

INT16			bitMap[MAX_DISKS][MAX_SECTORS] = {-1};

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

void read_Disk( INT16 disk_id, INT16 sector, char DATA[PGSIZE] ){
	INT32 	Temp = 0;
	INT32	Temp1 = 0;

	INT32 	DISK = disk_id;
	INT32	SECT = sector;

	//ID, Sector, and Data
	ZCALL( MEM_WRITE( Z502DiskSetID, &DISK) );
    while( Temp != DEVICE_FREE )  {
        ZCALL( Z502_IDLE() );
        ZCALL( MEM_WRITE( Z502DiskSetID, &DISK) );       
        ZCALL( MEM_READ( Z502DiskStatus, &Temp ) );
    }
    ZCALL( MEM_WRITE( Z502DiskSetSector, &SECT ) );
    ZCALL( MEM_WRITE( Z502DiskSetBuffer, (INT32*)DATA ) );
    //Specify a read
    Temp = 0;
    ZCALL( MEM_WRITE( Z502DiskSetAction, &Temp ) );
    //Start disk
    Temp = 0;                        // Must be set to 0
    ZCALL( MEM_WRITE( Z502DiskStart, &Temp ) );

    current_PCB->disk = disk_id;
    CALL( suspend_Process(-1, &Temp1) );
}

void write_Disk( INT16 disk_id, INT16 sector, char DATA[PGSIZE] ){
	INT32	Temp = 0;
	INT32	Temp1 = 0;

	INT32 DISK = (INT32) disk_id;
	INT32 SECT = (INT32) sector;

	//ID, Sector, and Data
	ZCALL( MEM_WRITE( Z502DiskSetID, &DISK ) );
	//Wait until Disk is free
	ZCALL( MEM_READ( Z502DiskStatus, &Temp ) );
   while( Temp != DEVICE_FREE )  {
        ZCALL( Z502_IDLE() );
        ZCALL( MEM_WRITE( Z502DiskSetID, &DISK ) );       
        ZCALL( MEM_READ( Z502DiskStatus, &Temp ) );
    }
    ZCALL( MEM_WRITE( Z502DiskSetSector, &SECT ) );

    ZCALL( MEM_WRITE( Z502DiskSetBuffer, (INT32*)DATA ) );
    //Specify a write
    Temp = 1;
    ZCALL( MEM_WRITE( Z502DiskSetAction, &Temp ) );
    //Start disk
    Temp = 0;                        // Must be set to 0
    ZCALL( MEM_WRITE( Z502DiskStart, &Temp ) );

    current_PCB->disk = disk_id;    
    CALL( suspend_Process( -1, &Temp1) );
}

INT32 diskHandler( INT32 diskStatus, INT32 disk ){
	INT32 count;
	CALL( count = wakeup_Disks(disk) );	

	switch(diskStatus){
		case(ERR_SUCCESS):
//			printf("ERR_SUCCESS\n");
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
	return count;
}

INT32 wakeup_Disks(INT32 disk){
	PCB_t 	*ptrCheck = pidList;
	INT32	count = 0;
	INT32	Temp = 0;

	while( ptrCheck != NULL ){
		if ( (ptrCheck->disk == disk) && 
			(ptrCheck->p_state == SUSPENDED_STATE) ){
			count++;
			ptrCheck->disk = -1;
			ptrCheck->p_state = READY_STATE;			
		}
		ptrCheck = ptrCheck->next;
	}
	//printf("WOKE UP %d\n", count);
	return count;
}

void get_emptyDisk( INT16 *disk, INT16 *sector){
	INT16 disks, sectors;

	for( disks = 1; disks <= MAX_DISKS; disk++ ){
		for( sectors = 1; sectors <= MAX_SECTORS; sectors++ ){
			if( bitMap[disks][sectors] != -1 ){
				bitMap[disks][sectors] = 1;
				*disk = disks;
				*sector = sectors;
				return;
			}
		}
	}
	*disk = -1;
	*sector = -1;
	return;
}
