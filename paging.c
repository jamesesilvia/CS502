/*
*	This file contains all routines that handle paging
*	as well as disks.
*
*	PAGE FAULT HANDLER
*	DISK WRITE
*	DISK READ
*	DISK HANDLER
*	
*	It also manages the disk bitMap as well as the
*	shadow table to keep track of disk and memory usage
*
*/

#include				"global.h"
#include				"syscalls.h"
#include				"protos.h"
#include				"string.h"

#include				"userdefs.h"

#define			MAXPGSIZE			1024

UINT16        	*Z502_PAGE_TBL_ADDR;
INT16         	Z502_PAGE_TBL_LENGTH;
INT16	      	SYS_CALL_CALL_TYPE;

//Shadow Table INIT
SHADOWTABLE_t	*shadowList = NULL;
//Disk Map INIT
INT16			bitMap[MAX_DISKS][MAX_SECTORS] = {0};
INT32			inc_shadow = 0;

/*
* HANDLE PAGE FAULT
*
*	On every page fault, this function is called. It finds a frame,
*	that is either not been used, or based on the least recently
*	used algorithm.
*
*	If the frame table is full and there is a page fault, additional
*	Memory management is required. If the system is trying to write 
*	to Memory, the original data must be written to disk and remembered
*	(via Shadow Table). If the system is reading from memory, but the 
*	page is not in a frame, the original data in the frame must be written
*	to disk, and the requesting page is written back in to memory.
*
*/
INT32 handlePaging( INT32 pageRequest ){
	INT32 		frame = -1;
	INT16 		call_type = -1;
	FRAMETABLE_t	*tableReturn;
	INT32		check = -1;
	
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
		if(call_type == SYSNUM_MEM_WRITE){
			//Write memory to disk 
			CALL( mem_toDisk(tableReturn, pageRequest) );
		}
		else if(call_type == SYSNUM_MEM_READ){
			//Write memory to disk, and then replace memory
			//with requesting data from disk
			CALL( mem_toDisk(tableReturn, pageRequest) ); 
			CALL( disk_toMem(tableReturn, pageRequest) );
		}
	}
	return frame;
}
// Helper function that moves memory to disk
// It adds the information to a Shadow table such that
// it can be brought back to in to memory at a later time
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

	//Read and Write Data from Memory
	char	DATA[PGSIZE];
	Z502_PAGE_TBL_ADDR[tableReturn->page] = tableReturn->frame;
	Z502_PAGE_TBL_ADDR[tableReturn->page] |= PTBL_VALID_BIT;
	ZCALL( MEM_READ( (tableReturn->page*PGSIZE), (INT32*)DATA) );
	//Clear page table, will be set back in fault handler.
	Z502_PAGE_TBL_ADDR[tableReturn->page] = 0;
	//Set new page and refTime in the page Table
	CALL( updatePage(pageRequest, tableReturn->frame) );
	//Write Data to Disk
	write_Disk( disk_id, sector, (char*)DATA);
}
// Helper function that moves disk to memory. It checks the 
// Shadow Table to ensure the page and processID line up,
// and then writes the data to memory.
void disk_toMem( FRAMETABLE_t *tableReturn, INT32 pageRequest ){
	SHADOWTABLE_t *ptrCheck;
	INT16	disk_id, sector;
	char	DATA[PGSIZE];
	memset(&DATA[0], 0, sizeof(DATA));
	

	INT32 ID = tableReturn->p_id;
	INT32 page = tableReturn->page;

	ptrCheck = shadowList;
	while(ptrCheck != NULL){
		// If the page returned from get_frame matched shadow Table
		// we must move the item in disk to memory.
		if( ptrCheck->page == page && ptrCheck->p_id == ID){
			//Read the Disk
			read_Disk( ptrCheck->disk, ptrCheck->sector, DATA);
			//Write to memory location
			Z502_PAGE_TBL_ADDR[pageRequest] = tableReturn->frame;
			Z502_PAGE_TBL_ADDR[pageRequest] |= PTBL_VALID_BIT;
			ZCALL( MEM_WRITE(page*PGSIZE, (INT32*)DATA) );
			rm_fromShadow( ptrCheck->count );
			return;
		}	
		ptrCheck = ptrCheck->next;
	}
	
}
// If page table is not full, this function will return the
// next available frame
INT32 get_emptyFrame( INT32 pageRequest ){
	FRAMETABLE_t *ptrCheck = pageList;
	while( ptrCheck != NULL ){
		if(ptrCheck->page == -1){
			ptrCheck->p_id = current_PCB->p_id;
			ptrCheck->page = pageRequest;
			CALL( updateTime(ptrCheck->frame) );
			FIFO++;

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
// If page table is full, it will return the item in pageTable that
// is least recently used (based on page stamp in table)
FRAMETABLE_t *get_fullFrame( INT32 pageRequest ){
	FRAMETABLE_t *ptrCheck = pageList;
	FRAMETABLE_t *tableReturn;

	INT32	smallestTime = -1;
	INT32	frame = -1;
	INT32	ID, page;
	INT32	check;
	long	Value;

	//Least Recently Used
	if( LRU == 1 ){
		while( ptrCheck != NULL ){
			if( smallestTime == -1){
				smallestTime = ptrCheck->refTime;
				tableReturn = ptrCheck;
			}
			else{
				if( smallestTime > ptrCheck->refTime ){
					CALL( check = checkref_Bit(ptrCheck->page) );
					if (check == 1){
						tableReturn = ptrCheck;
					}
					smallestTime = ptrCheck->refTime;					
				}
			}
		ptrCheck = ptrCheck->next;		
		}
		CALL( updateTime(tableReturn->frame) );
		return tableReturn;
	}
	//Random Frame
	else if ( RAND == 1 ){
		CALL( get_skewed_random_number( &Value, 63) );
		while( ptrCheck != NULL){
			if( Value == ptrCheck->frame ){
				return ptrCheck;
			}
			ptrCheck = ptrCheck->next;
		}
	}
	//FIFO
	else{
		while( ptrCheck != NULL ){
			if (ptrCheck->frame == (FIFO % PHYS_MEM_PGS) ){
				tableReturn = ptrCheck;
				FIFO++;
				return tableReturn;
			}
			ptrCheck = ptrCheck->next;			
		}
	}
}
// Add a shadow table entry
void add_to_Shadow( SHADOWTABLE_t *entry ){
	SHADOWTABLE_t *ptrCheck = shadowList;
	inc_shadow++;
	entry->count = inc_shadow;

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
// Remove shadow table entry
void rm_fromShadow( INT32 removeID ){
	SHADOWTABLE_t *ptrCheck = shadowList;
	SHADOWTABLE_t *ptrPrev = NULL;

	while(ptrCheck != NULL){
		if(ptrCheck->count == removeID){
			if(ptrPrev == NULL){
				shadowList = ptrCheck->next;
				free(ptrCheck);
				return;
			}
			else if (ptrCheck->next == NULL){
				ptrPrev->next = NULL;
				free(ptrCheck);
				return;
			}
			else{
				ptrPrev->next = ptrCheck->next;
				free(ptrCheck);
				return;
			}
		}
		ptrPrev = ptrCheck;
		ptrCheck = ptrCheck->next;
	}
}
// Helper function used to restamp the page in the
// frame Table as well as update the ID.
void updatePage(INT32 pageRequest, INT32 frame){
	FRAMETABLE_t * ptrCheck = pageList;

	while( ptrCheck != NULL){
		if (ptrCheck->frame == frame){
			ptrCheck->page = pageRequest;
//			CALL( ptrCheck->refTime = get_currentTime() );
			ptrCheck->p_id = current_PCB->p_id;
			return;
		}
		ptrCheck = ptrCheck->next;
	}
}
// Helper function used to restamp the time in the
// frameTable as well as update the ID.
void updateTime(INT32 frame){
	FRAMETABLE_t * ptrCheck = pageList;

	while( ptrCheck != NULL){
		if (ptrCheck->frame == frame){
			CALL( ptrCheck->refTime = get_currentTime() );
			ptrCheck->p_id = current_PCB->p_id;
			return;
		}
		ptrCheck = ptrCheck->next;
	}
}
// Helper function to ensure pageRequest is within
// proper limits. Terminates all process and children on
// error
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
/*
*
* DISK READ
*
*	Reads from disk based on input disk_id and sector.
*	It will wait until the disk is free to set the diskID,
*	sector, and disk buffer. It sets the current PCB's disk
*	item to the disk in use, starts the disk read, and 
*	Suspends the process. It will be woken up by the 
*	Event Handler (ISR).
*
*/
void read_Disk( INT16 disk_id, INT16 sector, char DATA[PGSIZE] ){
	INT32 	Temp = 0;
	INT32	Temp1 = 0;

	INT32 	DISK = disk_id;
	INT32	SECT = sector;
	memset(&DATA[0], 0, sizeof(DATA));

	if( SCHEDULEpo && (PRINT_COUNT % SCHEDULE_GRAN == 0) ){
		printState("DISK_R");
	}

	//ID, Sector, and Data
	ZCALL( MEM_WRITE( Z502DiskSetID, &DISK) );
	//Wait until Disk is free
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
/*
* DISK WRITE
*
*	Will write to disk based on disk_id and sector. It will wait 
*	until the disk is free to set the diskID, sector, and disk 
*	buffer. It sets the current PCB's disk item to the disk in use,
*	starts the disk write, and suspends the process. It will be
*	woken up by the Event Handler (ISR).
*
*/
void write_Disk( INT16 disk_id, INT16 sector, char DATA[PGSIZE] ){
	INT32	Temp = 0;
	INT32	Temp1 = 0;

	INT32 DISK = (INT32) disk_id;
	INT32 SECT = (INT32) sector;

	if( SCHEDULEpo && (PRINT_COUNT % SCHEDULE_GRAN == 0) ){
		printState("DISK_W");
	}

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
/*
* DISK HANDLER
*
*	This function is called on every interrupt called by
*	a disk. There are several cases than can be returned by
*	the interrupt, as shown. But they are not currently
*	required.
*
*	It calls wakeup_Disks, based on the disk that interrupted. 
*	This will wake up processes who are using the disk that
*	interrupted
*
*/
INT32 diskHandler( INT32 diskStatus, INT32 disk ){
	INT32 count;
	CALL( count = wakeup_Disks(disk) );	

	switch(diskStatus){
		case(ERR_SUCCESS):
//			printf("ERR_SUCCESS\n");
			break;
		case(ERR_BAD_PARAM):
			debugPrint("ERR_BAD_PARAM");
			break;
		case(ERR_NO_PREVIOUS_WRITE):
			debugPrint("ERR_NO_PREVIOUS_WRITE");
			break;
		case(ERR_DISK_IN_USE):
			debugPrint("ERR_DISK_IN_USE");
			break;
	}
	return count;
}
// Helper function that sets the process state to Ready,
// and clears the disk in use flag in the PCB
// It returns a wakeUp count
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
	return count;
}
// Helper function that returns the disk and sector
// that is free based on the disk bitMap 2D array
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
//Check Reference bit of PCB's PageTable
INT32 checkref_Bit( INT32 page, INT32 ID ){
	PCB_t * ptrCheck = pidList;
	while( ptrCheck != NULL ){
		if( (*ptrCheck->pageTable & PTBL_REFERENCED_BIT) >> 13 == 1){
			return 1;
		}
		ptrCheck = ptrCheck->next;
	}
	return 0;
}
