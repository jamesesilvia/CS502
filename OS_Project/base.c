/************************************************************************

        This code forms the base of the operating system you will
        build.  It has only the barest rudiments of what you will
        eventually construct; yet it contains the interfaces that
        allow test.c and z502.c to be successfully built together.      

        Revision History:       
        1.0 August 1990
        1.1 December 1990: Portability attempted.
        1.3 July     1992: More Portability enhancements.
                           Add call to sample_code.
        1.4 December 1992: Limit (temporarily) printout in
                           interrupt handler.  More portability.
        2.0 January  2000: A number of small changes.
        2.1 May      2001: Bug fixes and clear STAT_VECTOR
        2.2 July     2002: Make code appropriate for undergrads.
                           Default program start is in test0.
        3.0 August   2004: Modified to support memory mapped IO
        3.1 August   2004: hardware interrupt runs on separate thread
        3.11 August  2004: Support for OS level locking
************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include            "userdefs.h"

extern char			MEMORY[];  
extern BOOL			POP_THE_STACK;
extern UINT16			*Z502_PAGE_TBL_ADDR;
extern INT16			Z502_PAGE_TBL_LENGTH;
extern INT16			Z502_PROGRAM_COUNTER;
extern INT16			Z502_INTERRUPT_MASK;
extern INT32			SYS_CALL_CALL_TYPE;
extern INT16			Z502_MODE;
extern Z502_ARG			Z502_ARG1;
extern Z502_ARG			Z502_ARG2;
extern Z502_ARG			Z502_ARG3;
extern Z502_ARG			Z502_ARG4;
extern Z502_ARG			Z502_ARG5;
extern Z502_ARG			Z502_ARG6;

extern void          *TO_VECTOR [];
extern INT32         CALLING_ARGC;
extern char          **CALLING_ARGV;

PCB_t				*pidList = NULL;
PCB_t				*timerList = NULL;
PCB_t				*current_PCB = NULL;
EVENT_t				*eventList = NULL;

FRAMETABLE_t		*pageList = NULL;

char                 *call_names[] = { "mem_read ", "mem_write",
                            "read_mod ", "get_time ", "sleep    ", 
                            "get_pid  ", "create   ", "term_proc", 
                            "suspend  ", "resume   ", "ch_prior ", 
                            "send     ", "receive  ", "disk_read",
                            "disk_wrt ", "def_sh_ar" };

INT32			inc_pid = 0;
INT32			total_pid = 0;
INT32			event_count = 0;
INT32			inc_event = 0;
INT32           LRU = 0;
INT32           FIFO = 0;
INT32           RAND = 0;


INT32			CREATEpo = 0;
INT32			TERMINATEpo = 0;
INT32			SLEEPpo = 0;
INT32			RESUMEpo = 0;
INT32			SUSPENDpo = 0;
INT32			PRIORITYpo = 0;
INT32			SENDpo = 0;
INT32			RECEIVEpo = 0;
INT32			WAKEUPpo = 0;
INT32			FAULTpo = 0;
INT32           SCHEDULEpo = 0;
INT32           DISKpo = 0;
INT32           MEMpo = 0;

INT32           MEM_GRAN = 0;
INT32           SCHEDULE_GRAN = 0;
INT32           FAULT_GRAN = 0;
INT32           PRINT_COUNT = 0;

/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.
************************************************************************/
void    interrupt_handler( void ) {
		INT32			device_id;
        INT32			Status;
        INT32			Index = 0;
        INT32			currentTime;
        INT32			success;
        PCB_t			*switchPCB;
		INT32			wokenUp;
		INT32			sleeptime;

        // Get cause of interrupt
        ZCALL( MEM_READ(Z502InterruptDevice, &device_id ) );
        //Loop interrupts until there are no more, add all to the event Queue
        while(device_id != -1){
            // Set this device as target of our query
            ZCALL( MEM_WRITE(Z502InterruptDevice, &device_id ) );
            // Now read the status of this device
            ZCALL( MEM_READ(Z502InterruptStatus, &Status ) );
    
            CALL( add_to_eventQueue(&device_id, &Status) ); 
            ZCALL( MEM_WRITE(Z502InterruptClear, &Index ) );

            ZCALL( MEM_READ(Z502InterruptDevice, &device_id ) );
        }
    return;
}                                       /* End of interrupt_handler */

/************************************************************************
    FAULT_HANDLER
        The beginning of the OS502.  Used to receive hardware faults.
************************************************************************/

void    fault_handler( void ) {
    INT32           device_id;
    INT32           status;
    INT32           Index = 0;
    INT32           frame = -1;
    INT16           call_type;
    char            DATA[PGSIZE];

    call_type = (INT16)SYS_CALL_CALL_TYPE;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

       
    //Switch case on fault device ID.
    //Terminate Process and children on CPU and privledge instruction
    switch(device_id){
        case(CPU_ERROR):
            //FAULT PRINTOUT
            if ( FAULTpo && (PRINT_COUNT % FAULT_GRAN == 0) && DEBUGFLAG){
                printf("CPU ERROR\n");
                printf("TERMINATE PROCESS AND CHILDREN\n");               
            }
            terminate_Process(-2, &Index);
            MEM_WRITE(Z502InterruptClear, &Index );
            break;

        case(PRIVILEGED_INSTRUCTION):
            //FAULT PRINTOUT
            if ( FAULTpo && (PRINT_COUNT % FAULT_GRAN == 0) && DEBUGFLAG){
                printf("ERROR: PRIVILEDGED INSTRUCTION\n");
                printf("TERMINATE PROCESS AND CHILDREN\n");
            }                        
            terminate_Process(-2, &Index);
            MEM_WRITE(Z502InterruptClear, &Index );
            break;

        case(INVALID_MEMORY):
            //FAULT PRINTOUT
            if ( FAULTpo && (PRINT_COUNT % FAULT_GRAN == 0) && DEBUGFLAG){
                printf("HANDLING PAGE FAULT: %d\n", status);
            } 
            //Check page request
            CALL( check_pageSize( status ) );
            //Get frame and manage memory/disks
            CALL( frame = handlePaging( status ) );

            //FAULT PRINTOUT
            if ( FAULTpo && (PRINT_COUNT % FAULT_GRAN == 0) && DEBUGFLAG){
                printf("FRAME RETURNED: %d\n", frame );
            }

            //Setup address with frame and valid bit
            Z502_PAGE_TBL_ADDR[status] = frame;
            Z502_PAGE_TBL_ADDR[status] |= PTBL_VALID_BIT;
            
            //Based on Call Type
            //MEMREAD or MEMWRITE
           if( call_type == SYSNUM_MEM_READ ){          	 
                ZCALL( MEM_READ( (INT32) Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR ) );
            }
            else if( call_type == SYSNUM_MEM_WRITE ){
                ZCALL( MEM_WRITE( (INT32) Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR ) );
            }
            //Print memory that is being used.
            if ( MEMpo && (PRINT_COUNT % MEM_GRAN == 0) && DEBUGFLAG){
                printMemory();
            }
            break;
    }

    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}                                       /* End of fault_handler */


/************************************************************************
    OS_SWITCH_CONTEXT_COMPLETE
        The hardware, after completing a process switch, calls this routine
        to see if the OS wants to do anything before starting the user
        process.
************************************************************************/

void    os_switch_context_complete( void )
    {
    static INT16        do_print = FALSE;
    INT16               call_type;
    MSG_t*              Message;

    PRINT_COUNT++;

    //Set the page table address to the current PCB's table    
    Z502_PAGE_TBL_ADDR = current_PCB->pageTable;
    Z502_PAGE_TBL_LENGTH = VIRTUAL_MEM_PGS;

    call_type = (INT16)SYS_CALL_CALL_TYPE;
    if ( do_print == TRUE )
    {
        printf( "os_switch_context_complete  called before user code.\n");
        do_print = FALSE;
    }

    //If receive message called, get current PCB message from inbox
    //Return to the OS for check
    if (call_type == SYSNUM_RECEIVE_MESSAGE){
        //Z502_ARG2.PTR = *message
        //Z502_ARG4.PTR = *Length
        //Z502_ARG5.PTR = *send_ID   
        CALL( get_msg_Inbox((char *)Z502_ARG2.PTR,(INT32 *)Z502_ARG4.PTR,
            (INT32 *)Z502_ARG5.PTR) );
   }
}                               /* End of os_switch_context_complete */

/************************************************************************
    OS_INIT
        This is the first routine called after the simulation begins.  This
        is equivalent to boot code.  All the initial OS components can be
        defined and initialized here.
************************************************************************/

void    os_init( void )
    {
    void                *next_context;
    INT32               i;
    void                *procPTR;
    
   /* Demonstrates how calling arguments are passed thru to here       */
    printf( "Program called with %d arguments:", CALLING_ARGC );
    printf( "\n" );

    /*          Setup so handlers will come to code in base.c           */
    TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR]   = (void *)interrupt_handler;
    TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)fault_handler;
    TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR]  = (void *)svc;


    /*  Determine if the switch was set, and if so go to demo routine.  */
    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "sample" ) == 0 ) )
        {
        ZCALL( Z502_MAKE_CONTEXT( &next_context,
                                        (void *)sample_code, KERNEL_MODE ));
        ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &next_context ));
    }                   /* This routine should never return!!           */


    /*          Based on user input, select test appropriately          */
    /*   Configure global variables for state printout, test dependent  */
    if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test0" ) == 0 ) )
        procPTR = test0;
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1a" ) == 0 ) ){
        procPTR = test1a;
        CREATEpo = 1;
        SLEEPpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1b" ) == 0 ) ){
        procPTR = test1b;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1c" ) == 0 ) ){
        procPTR = test1c;
        CREATEpo = 1;
        SLEEPpo = 1;
        WAKEUPpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1d" ) == 0 ) ){
        procPTR = test1d;
        CREATEpo = 1;
        SLEEPpo = 1;
        WAKEUPpo = 1;
        TERMINATEpo = 1;
    }        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1e" ) == 0 ) ){
        procPTR = test1e;
        CREATEpo = 1;
        SUSPENDpo = 1;
        RESUMEpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1f" ) == 0 ) ){
        procPTR = test1f;
        CREATEpo = 1;
        SUSPENDpo = 1;
        RESUMEpo = 1;
        TERMINATEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1g" ) == 0 ) ){
        procPTR = test1g;
        CREATEpo = 1;
        PRIORITYpo = 1;
        TERMINATEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1h" ) == 0 ) ){
        procPTR = test1h;
        CREATEpo = 1;
        PRIORITYpo = 1;
        TERMINATEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1i" ) == 0 ) ){
        procPTR = test1i;
        CREATEpo = 1;
        PRIORITYpo = 1;
        SENDpo = 1;
        RECEIVEpo = 1;
        TERMINATEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1j" ) == 0 ) ){
        procPTR = test1j;
        CREATEpo = 1;
        PRIORITYpo = 1;
        SENDpo = 1;
        RECEIVEpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1k" ) == 0 ) ){
        procPTR = test1k;
        CREATEpo = 1;
        FAULTpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1l" ) == 0 ) ){
        procPTR = test1l;
        CREATEpo = 1;
        PRIORITYpo = 1;
        SUSPENDpo = 1;
        RESUMEpo = 1;
        SENDpo = 1;
        RECEIVEpo = 1;
        TERMINATEpo = 1;
    }
        
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test1m" ) == 0 ) ){
        procPTR = test1m;
        CREATEpo = 1;
        PRIORITYpo = 1;
        SUSPENDpo = 1;
        RESUMEpo = 1;
        SENDpo = 1;
        RECEIVEpo = 1;
        TERMINATEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2a" ) == 0 ) ){
        procPTR = test2a;

        MEM_GRAN = 1;
        FAULT_GRAN = 1;
        FAULTpo = 1; 
        MEMpo = 1;      
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2b" ) == 0 ) ){
        procPTR = test2b;

        MEM_GRAN = 1;
        FAULT_GRAN = 1;
        FAULTpo = 1;  
        MEMpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2c" ) == 0 ) ){
        procPTR = test2c;

        FAULT_GRAN = 10;
        SCHEDULE_GRAN = 1;
        FAULTpo = 1;
        SCHEDULEpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2d" ) == 0 ) ){
        procPTR = test2d;

        FAULT_GRAN = 5;
        SCHEDULE_GRAN = 5;
        SCHEDULEpo = 1;
        FAULTpo = 1;
    }
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2e" ) == 0 ) ){
        procPTR = test2e;
        FAULT_GRAN = 5;
        SCHEDULE_GRAN = 5;
        MEM_GRAN = 5;
        LRU = 1;

        FAULTpo = 1;
        MEMpo = 1;
        SCHEDULEpo = 1;
    }
    //FIFO
    else if (( CALLING_ARGC > 1 ) && ( strcmp( CALLING_ARGV[1], "test2f" ) == 0 ) ){
        procPTR = test2f;
        
        FAULT_GRAN = 10;
        MEM_GRAN = 10;
        FAULTpo = 1;
        MEMpo = 1;
    }
    //Least Recently Used
    else if (( CALLING_ARGC > 1) && (strcmp( CALLING_ARGV[1], "test2h" ) == 0 ) ){
        procPTR = test2f;
        LRU = 1;

        FAULT_GRAN = 10;
        MEM_GRAN = 10;
        FAULTpo = 1;
        MEMpo = 1;
    }
    //Random Frame
    else if (( CALLING_ARGC > 1) && (strcmp( CALLING_ARGV[1], "test2i" ) == 0 ) ){
        procPTR = test2f;
        RAND = 1;

        FAULT_GRAN = 10;
        MEM_GRAN = 10;
        FAULTpo = 1;
        MEMpo = 1;
    }
 
    else{
        printf("NO TEST SELECTED, HALT!!!\n");
        Z502_HALT();
    }

    //Initialize Page Table
    INT32 frame = 0;
    while (frame < PHYS_MEM_PGS){ 
        FRAMETABLE_t *Table = (FRAMETABLE_t *)(malloc(sizeof(FRAMETABLE_t)));
        Table->p_id = -1;
        Table->page = -1;
        Table->page = -1;
        Table->frame = frame;
        Table->refTime = -1;
        Table->next = NULL;

        FRAMETABLE_t *ptrCheck = pageList;     
        if (ptrCheck == NULL){
            pageList = Table;
        }
        else{
            while (ptrCheck->next != NULL){
                ptrCheck = ptrCheck->next;
            }
            ptrCheck->next = Table;
        }
        frame++;
    }
    
    // Create a new process, and switch to it.
    CALL( OS_Create_Process(CALLING_ARGV[1], procPTR, 0, &i, &i, 1) );
}                                               /* End of os_init       */

/*
* OS CREATE PROCESS
*
*   This function builds a PCB following several steps of error checking
*   It the places the PCB on the Ready Queue as the PCB is built and ready
*   to run. If the SWITCH variable is set (first PCB only), the context
*   is both made, and also switched to. Otherwise, the context is made
*   and the function returns.
*
*/
INT32    OS_Create_Process( char * name, void * procPTR,
        INT32 priority, INT32 *pid, INT32 *error, INT32 SWITCH){

    //Check for illegal priority
    if (priority < 0){
        debugPrint("OS CREATE ERROR: ILLEGAL PRIORITY");
        (*error) = ERR_BAD_PARAM;
        return -1;
    }
    //Check if input name is a duplicate
    if (pidList != NULL){
        if(check_name(&pidList, name) == 0){
            debugPrint("OS CREATE ERROR: DUPLICATE NAME");
            (*error) = ERR_BAD_PARAM;
            return -1;
        }    
    }
    //Check if MAX_PIDs (100) exceeded
    if (total_pid >= MAX_PIDs){
        debugPrint("OS CREATE ERROR: EXCEEDED MAX NUMBER OF PIDs");
        (*error) = ERR_BAD_PARAM;
        return -1;
    }    
    inc_pid++;

    //Checks are clear, make space and build PCB
    PCB_t *PCB = (PCB_t *)(malloc(sizeof(PCB_t)));
    PCB->p_state = NEW_STATE;
    PCB->p_id = inc_pid;
    PCB->p_time = 0;
    memset(PCB->p_name, 0, MAX_NAME+1);
    strcpy(PCB->p_name,name);
    PCB->p_priority = priority;
    PCB->next = NULL;
    PCB->prev = NULL;
    PCB->msg_state = READY_MSG;
    PCB->disk = -1;
    PCB->msg_count = 0;
    
    //Set all values in pageTable to zero
    memset(PCB->pageTable, 0, VIRTUAL_MEM_PGS+1);

    //If there is a process currently running (not first PCB)
    //Set the new PCB's parent to the running process
    if (current_PCB != NULL) PCB->p_parent = current_PCB->p_id;
    
    //Add to Ready List
    CALL( add_to_readyQueue(&pidList, PCB) );

    //Return Success and the ID to the OS
    (*error) = ERR_SUCCESS;
    (*pid) = PCB->p_id;

    //STATE PRINT OUT
    if( CREATEpo && DEBUGFLAG){
        SP_setup( SP_NEW_MODE, inc_pid );
        SP_setup( SP_PRIORITY_MODE, priority );
        SP_print_header();
        SP_print_line();
        printReady();
    }
    
    //SWITCH if first PCB, else make the context and return
    if (SWITCH == 1) CALL( make_switch_Savecontext(PCB, procPTR) );
    if (SWITCH != 1) CALL( make_context(PCB, procPTR) );

    return 0;                                             
}                                       /* End off OS_Create_Process */

/************************************************************************
    SVC
        The beginning of the OS502.  Used to receive software interrupts.
        All system calls come to this point in the code and are to be
        handled by the student written code here.
************************************************************************/

void    svc( void ) {
    INT16               call_type;
    static INT16        do_print = 0;
    INT32                Time;
    INT32                 sleepTime;
    INT32                currentTime;
    PCB_t                *temp;
    PCB_t                *switchPCB;

    call_type = (INT16)SYS_CALL_CALL_TYPE;

    //Do print set to 0. Do nothing.
    if ( do_print > 0 ) {
        printf( "SVC handler: %s %8ld %8ld %8ld %8ld %8ld %8ld\n",
                call_names[call_type], Z502_ARG1.VAL, Z502_ARG2.VAL, 
                Z502_ARG3.VAL, Z502_ARG4.VAL, 
                Z502_ARG5.VAL, Z502_ARG6.VAL );
        do_print--;
    }
    //Check ISR event_count, Handle BEFORE system calls.
    // CALLS THE EVENT HANDLER
    if ( (event_count < 0) && (call_type != SYSNUM_TERMINATE_PROCESS) ){
        CALL( eventHandler() );    
    }
    
    //Switch on SYS CALL TYPE
    switch (call_type){
        //Get time of day
        case SYSNUM_GET_TIME_OF_DAY:
            Time = get_currentTime();
            *(INT32 *)Z502_ARG1.PTR = Time;
            break;
        //Terminate a process
        case SYSNUM_TERMINATE_PROCESS:
            CALL( terminate_Process( (INT32)Z502_ARG1.VAL, (INT32*)Z502_ARG2.PTR ) );
            break;
        //Sleep Call
        case SYSNUM_SLEEP:
            //Get sleep and current time
            CALL( sleepTime = Z502_ARG1.VAL );
            CALL( currentTime = get_currentTime() );
            //PCB wakeUp time equal to their sum
            CALL( current_PCB->p_time = ( sleepTime + currentTime ) );
            //Move to the timer Queue
            CALL( readyQueue_to_timerQueue( current_PCB->p_id  ) );

            //State print out
            if ( SLEEPpo && DEBUGFLAG){
                SP_setup( SP_TARGET_MODE, current_PCB->p_id );
                SP_setup( SP_WAITING_MODE, current_PCB->p_id );
                SP_print_header();
                SP_print_line();
                printTimer();       
            }

            //Start the timer if items are sleeping
            //Get the smallest wakeUp time, sleeptime = wakeUp - current
            //This value is returned by checkTimer
            CALL( sleepTime = checkTimer(currentTime) );
            if (sleepTime > 0) CALL( Start_Timer( sleepTime ) );            

            //Switch to New PCB or IDLE
            CALL( switchPCB = get_readyPCB() );
            if (switchPCB == NULL) ZCALL( EVENT_IDLE() );
            if (switchPCB != NULL) CALL( switch_Savecontext(switchPCB) );
            break;
        //Create Process
        case SYSNUM_CREATE_PROCESS:
            CALL( OS_Create_Process((char*)Z502_ARG1.PTR, (void *)Z502_ARG2.PTR,
                (INT32)Z502_ARG3.VAL,(INT32*)Z502_ARG4.PTR, (INT32*)Z502_ARG5.PTR, 0) );
            break;
        //Get Process ID
        case SYSNUM_GET_PROCESS_ID:
            CALL( get_PCB_ID(&pidList, (char *)Z502_ARG1.PTR,
                    (INT32 *)Z502_ARG2.PTR, (INT32 *)Z502_ARG3.PTR) );
            break;
        //Suspend Process
        case SYSNUM_SUSPEND_PROCESS:
            CALL( suspend_Process((INT32)Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR) );
            break;
        //Resume Process
        case SYSNUM_RESUME_PROCESS:
            CALL( resume_Process((INT32)Z502_ARG1.VAL, (INT32 *)Z502_ARG2.PTR) );
            break;
        //Change Priority
        case SYSNUM_CHANGE_PRIORITY:
            CALL( change_Priority((INT32)Z502_ARG1.VAL,(INT32)Z502_ARG2.VAL,
                (INT32*)Z502_ARG3.PTR));
            break;
        //Send Message
        case SYSNUM_SEND_MESSAGE:
            CALL( send_Message((INT32)Z502_ARG1.VAL,(char *)Z502_ARG2.PTR,
                (INT32)Z502_ARG3.VAL,(INT32 *)Z502_ARG4.PTR) );
            break;
        //Receive Message
        case SYSNUM_RECEIVE_MESSAGE:            
            CALL( receive_Message((INT32)Z502_ARG1.VAL,(char *)Z502_ARG2.PTR,
                (INT32)Z502_ARG3.VAL,(INT32 *)Z502_ARG4.PTR,(INT32 *)Z502_ARG5.PTR,
                (INT32 *)Z502_ARG6.PTR) );
            break;
        //Disk Read
        case SYSNUM_DISK_READ:
            CALL( read_Disk( Z502_ARG1.VAL, Z502_ARG2.VAL, Z502_ARG3.PTR) );
            break;
        //Disk Write
        case SYSNUM_DISK_WRITE:
            CALL( write_Disk( Z502_ARG1.VAL, Z502_ARG2.VAL, Z502_ARG3.PTR) );
            break;
        //HALT IF CALL TYPE NOT RECOGNIZED
        default:
            printf("ERROR! call_type not recognized!\n");
            printf("Entered Call_Type is %i\n", call_type);
            ZCALL( Z502_HALT() );
            break;
    }                                            // End of switch call_type
}                                               // End of svc
// Helper function to check if duplicate name exists
// on the Ready Queue
INT32 check_name( PCB_t **ptrFirst, char *name ){
    ZCALL( lockReady() );
    PCB_t *ptrCheck = *ptrFirst;

    while (ptrCheck != NULL){
        if(strcmp(ptrCheck->p_name, name) == 0){

            ZCALL( unlockReady() );
            return 0;
        }
        ptrCheck = ptrCheck->next;
    }
    ZCALL( unlockReady() );
    return 1;
}
// Helper function that Starts the Timer for interrupt
void Start_Timer(INT32 Time) {    
    INT32        Status;
    ZCALL ( HW_lock() );
    ZCALL( MEM_WRITE( Z502TimerStart, &Time ) );
    ZCALL( MEM_READ( Z502TimerStatus, &Status ) );
    ZCALL ( HW_unlock() );
}
/*
* GET TIME OF DAY
*
*   This routine checks the Z502 Clock status and
*   returns the timer
*
*/
INT32 get_currentTime( void ) {
    INT32     currentTime;
    ZCALL ( HW_lock() );
    ZCALL( MEM_READ( Z502ClockStatus, &currentTime ) );
    ZCALL ( HW_unlock() );
    return currentTime;
}
/*
* EVENT HANDLER
*
*   This function handles all interrupts, outside of the
*   fast interrupt handler. This was so the threaded ISR
*   and SVC routine do not cause issues with each other.
*   This function is called at the START of svc, and handles
*   all interrupts prior to handling system calls.
*
*/
void eventHandler ( void ) {
    EVENT_t 		*ptrCheck = eventList;

    INT32			interrupt;
    INT32			eventID;    
    INT32			currentTime;
    INT32			timeUp = 0;
    INT32			diskUp = 0;
    INT32			sleeptime;    

    //Remove all Events from Queue
    while (ptrCheck != NULL){
        interrupt = ptrCheck->device_ID;
        eventID = ptrCheck->id;
        CALL( rm_from_eventQueue(eventID) );
        switch(interrupt){
            //TIMER INTERRUPT
            case(TIMER_INTERRUPT):
                //Get current CPU Time  
                CALL( currentTime = get_currentTime() );
                //Wake up all WAITING items that are before currentTime
                CALL( timeUp = wake_timerList(currentTime) );
                //Get sleeptime
                CALL( sleeptime = checkTimer (currentTime) );
                //There are more items on timerQueue
                //Start the new timer
                if( sleeptime > 0) CALL( Start_Timer(sleeptime) );
                break;
            //DISK INTERRUPT
            case(DISK_INTERRUPT_DISK1):
            case(DISK_INTERRUPT_DISK2):     
            case(DISK_INTERRUPT_DISK3):
            case(DISK_INTERRUPT_DISK4):
            case(DISK_INTERRUPT_DISK5):
            case(DISK_INTERRUPT_DISK6):
            case(DISK_INTERRUPT_DISK7):
            case(DISK_INTERRUPT_DISK8):
            case(DISK_INTERRUPT_DISK9):
            case(DISK_INTERRUPT_DISK10):
            case(DISK_INTERRUPT_DISK11):
            case(DISK_INTERRUPT_DISK12):
                //Call disk Handler for all disk interrupts
                CALL( diskUp = diskHandler(ptrCheck->Status, interrupt - 4) );
                diskUp += diskUp;
        break;
        }
        ptrCheck = ptrCheck->next;
    }
    //If any items were wokenUp from timer or disks, check for context switch
    if( diskUp > 0 || timeUp > 0){
        switch_Context();
    } 

}
// Helper function, that is used to IDLE
// If there are no events, Z502 IDLE is called,
// otherwise the EVENT HANDLER is called
void EVENT_IDLE ( void ) {
    if (event_count == 0) CALL( Z502_IDLE() );
    CALL( eventHandler() );
}
// Helper function used to switch contexts
void switch_Context ( void ){
    PCB_t			*switchPCB;

    if (total_pid <= 0) ZCALL( Z502_HALT() );
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
