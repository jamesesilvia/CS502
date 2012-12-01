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

UINT16        *Z502_PAGE_TBL_ADDR;
INT16         Z502_PAGE_TBL_LENGTH;
INT16	      SYS_CALL_CALL_TYPE;

SHADOWTABLE_t	*shadowList = NULL;
INT16		bitMap[MAX_DISKS][MAX_SECTORS] = {0};

INT32 handlePaging( INT32 pageRequest, INT32* full ){
	INT32 		frame = -1;
	INT16 		call_type = -1;
	FRAMETABLE_t	*tableReturn;
	
	call_type = (INT16)SYS_CALL_CALL_TYPE;

	//Check for empty frame
	CALL( frame = get_emptyFrame(pageRequest) );
	if (frame != -1) return frame;

	//Page Table is full
	//Check table for next spot
	CALL( tableReturn = get_fullFrame(pageRequest) );
	frame = tableReturn->frame;
	//HALT IF SOMETHING GOES WRONG
	if (frame == -1){
		printf("SOMETHING WENT WRONG IN PAGE HANDLE\n");
		ZCALL( Z502_HALT );
	}
	//Full Frame, handle memory and disks
	else{
		*full = 1;
		if(call_type == SYSNUM_MEM_WRITE){
			CALL( mem_toDisk(tableReturn, pageRequest) );
		}
		else if(call_type == SYSNUM_MEM_READ){
			CALL( mem_toDisk(tableReturn, pageRequest) ); 
			CALL( disk_toMem(tableReturn, pageRequest) );
		}
	}
	return frame;
}
void mem_toDisk( FRAMETABLE_t *tableReturn, INT32 pageRequest ){

	//Write current frame to disk
	INT16	disk_id, sector;
	//Find empty sector
	CALL( get_emptyDisk(&disk_id, &sector) );
	//Write info to shadow table
	SHADOWTABLE_t *entry = (SHADOWTABLE_t *)(malloc(sizeof(SHADOWTABLE_t)));
	entry->p_id = tableReturn->p_id;
	entry->page = tableReturn->page;
	entry->frame = tableReturn->frame;
	entry->disk = disk_id;
	entry->sector = sector;
	entry->next = NULL;
	CALL( add_to_Shadow(entry) );

	//Read and Write Data
	char	DATA[PGSIZE];
	Z502_PAGE_TBL_ADDR[tableReturn->page] = tableReturn->frame;
	Z502_PAGE_TBL_ADDR[tableReturn->page] |= PTBL_VALID_BIT;

	ZCALL( MEM_READ( (tableReturn->page*PGSIZE), (INT32*)DATA) );
	//Clear page table
	Z502_PAGE_TBL_ADDR[tableReturn->page] = 0;
	//set new page
	CALL( updatePage(pageRequest, tableReturn->frame) );
	//write Data to Disk
	write_Disk( disk_id, sector, (char*)DATA);
}
void disk_toMem( FRAMETABLE_t *tableReturn, INT32 pageRequest ){
	SHADOWTABLE_t *ptrCheck;
	INT16	disk_id, sector;
	char	DATA[PGSIZE];

	INT32 ID = tableReturn->p_id;
	INT32 page = tableReturn->page;

	ptrCheck = shadowList;
	while(ptrCheck != NULL){
		if( ptrCheck->page == page && ptrCheck->p_id == ID){
			read_Disk( ptrCheck->disk, ptrCheck->sector, DATA);

			Z502_PAGE_TBL_ADDR[pageRequest] = tableReturn->frame;
			Z502_PAGE_TBL_ADDR[pageRequest] |= PTBL_VALID_BIT;
			ZCALL( MEM_WRITE(page*PGSIZE, (INT32*)DATA) );
			return;
		}	
		ptrCheck = ptrCheck->next;
	}
	
}

INT32 get_emptyFrame( INT32 pageRequest ){
	FRAMETABLE_t *ptrCheck = pageList;

	while( ptrCheck != NULL ){
		if(ptrCheck->page == -1){
			ptrCheck->p_id = current_PCB->p_id;
			ptrCheck->page = pageRequest;
			ptrCheck->refTime = get_currentTime();
			//Set the address and valid bit of page error
            Z502_PAGE_TBL_ADDR[pageRequest] = ptrCheck->frame;
            Z502_PAGE_TBL_ADDR[pageRequest] |= PTBL_VALID_BIT;
			return ptrCheck->frame;
		}
		ptrCheck = ptrCheck->next;
	}
	//Table is Full
	return -1;
}

FRAMETABLE_t *get_fullFrame( INT32 pageRequest ){
	FRAMETABLE_t *ptrCheck = pageList;
	FRAMETABLE_t *tableReturn;

	INT32	smallestTime = -1;
	INT32	frame = -1;
	INT32	ID, page;

	while( ptrCheck != NULL ){
		if( smallestTime == -1){
			smallestTime = ptrCheck->refTime;
			tableReturn = ptrCheck;
		}
		else{
			if( smallestTime > ptrCheck->refTime ){
				smallestTime = ptrCheck->refTime;
				tableReturn = ptrCheck;
			}
		}
		ptrCheck = ptrCheck->next;
	}
	return tableReturn;
}
void add_to_Shadow( SHADOWTABLE_t *entry ){
	SHADOWTABLE_t *ptrCheck = shadowList;

	//First Entry
	if( ptrCheck == NULL){
		shadowList = entry;
		return;
	}
	//Add to end of list
	while( ptrCheck->next != NULL){
		ptrCheck = ptrCheck->next;
	}
	ptrCheck->next = entry;
}

void updatePage(INT32 pageRequest, INT32 frame){
	FRAMETABLE_t * ptrCheck = pageList;

	while( ptrCheck != NULL){
		if (ptrCheck->frame == frame){
			ptrCheck->page = pageRequest;
			ptrCheck->refTime = get_currentTime();
			ptrCheck->p_id = current_PCB->p_id;
		}
		ptrCheck = ptrCheck->next;
	}
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
			if( bitMap[disks][sectors] == 0 ){
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
