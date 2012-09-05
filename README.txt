James Silvia

This is an OS design course that will be made up of several modules.

There will be several tests that build upon each other, with a goal
of developing an OS capable of handling several user requests. In its
original state, there was a thin kernel built, with simulated HW.

Tests Complete:
Test0

In Progress:
Test1a
	- Added os.c with similar OS functions
		. Implemented OS_Create_Process
	- Added userdefs.h
		. Created PCB_t typedef for process control block
	- To Do:
		. Start_Timer and sleep functionality
		. Timer Queue
		. Interrupt_Handler

Build Status:
Compiled Successfully

