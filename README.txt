James Silvia
Project Phase 1

SUCCESSFUL TESTS:

Test1a - 100%
Test1b - 100%
Test1c - 100%
Test1d - 100%
Test1e - 100%
Test1f - 100%
Test1g - 100%
Test1h - 100%
Test1i - 100%
Test1j - 100%
Test1k - 100%

Test1e WILL HANG because I made the design decision to allow
suspension of the running process. THIS IS OK. This is because the
running process suspends itself, and there are no other processes to
switch to. Therefore it IDLEs forever.

Test1l and Test 1m will run to completion.
Test1l does not have any of the logic built in, many calls fail.
Test1m runs to completion based on the current Operating System Design

BUILD INSTRUCTIONS:

The compiled code is not included. 
Running the "make" command in the root directory will compile the code,
and it will be output as the file 'os'.

Run the code by "./os test1a"
Possible inputs: test1a, test1b, test1c, test1d, test1e, test1f, test1g
	test1h, test1i, test1k, test1l, test1m

CURRENT CONFIGURATION:

It is currently configured to NOT printout any of the state printouts or 
debug printouts. The outputs to running each test can be be viewed in the files
with an file extension of OUT.

These files were built by piping the prints to a file via:
	./os test1a > Test1a.OUT

To ENABLE all of the printouts and state printer, edit the userdefs.h file
and set the "DEBUGFLAG" value from 0 to 1.

IMPORTANT ITEMS TO TRACE:

Each of the state printouts vary between each of the tests. This was to show 
that the functionality of each test was working properly. For example. Test1c 
and Test1d focus on sleeping and waking up processes. Test1i and Test1j focus
on sending and receiving message.

Queues will be print out and labeled in the .OUT file. I have included below 
important values that are also in userdefs.h. These can be seen changing throughout
the printouts and Queue updates.

// Process State
#define			NEW_STATE				90
#define			READY_STATE				91
#define			RUNNING_STATE			92
#define			WAITING_STATE			93
#define			SUSPENDED_STATE			94
//Message Status States
#define			RECEIVE_MSG 			60
#define			SEND_MSG 				61
#define			READY_MSG				62
#define			RECV_SUS_MSG			63

GUIDE TO THE CODE:

base.c
	Contains FAULT, and INTERRUPT Handlers
	os_switch_context_complete
	os_in_it
	OS CREATE PROCESS
	SVC
	GET TIME OF DAY
	EVENT HANDLER
	EVENT_IDLE
	Along with any relevant helper functions
contextHelpers.c
	Includes all make, switch, save, and kill context functions
debug.c
	Includes all queue, message printouts, and debugPrint function
locks.c
	Contains all the locks used for the queues and timer
messageHandle.c
	Contains SEND and RECEIVE MESSAGE
	Along with any relevant helper functions used
pcbHandle.c
	Contains UPDATE PRIORITY
	Along with any relevant helper functions used
processHandle.c
	Contains GET PROCESS ID
			TERMINATE PROCESS
			SUSPEND PROCESS
			RESUME PROCESS
	Along with any relevant helper functions used
queues.c
	Manages ALL queues including, add, remove, move
	Contains get_ready_PCB function for switching contexts
	Contains wake_timerList to move from timer to ready Queue
	Contains checkTimer that returns sleepTime from the timer Queue
sorts.c
	Functions to sort the Timer and Ready Queue
state_printer.c
	Used to print important information
*.OUT
	Contains all printouts for each test run
userdefs.h
	Contains all user definitions, and the DEBUGFLAG
