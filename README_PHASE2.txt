James Silvia
Project Phase 2

SUCCESSFUL TESTS:
Test2a - 100%
Test2b - 100%
Test2c - 100%
Test2d - 100%
Test2e - 100%
Test2f - 100%
ADDITIONAL TESTS:
Test2h - 100%
Test2i - 100%

All results with printouts are included in any file labeled *.OUT.

Currently, the code has been built to NOT printout ANY printouts.
This can be updated to display the prints shown in the *.OUT files
by setting the DEBUGFLAG to TRUE.

Page Replacement Algorithms:
FIFO - Test2f
LRU Approx - Test2h
Random - Test2i

BUILD INSTRUCTIONS:

The compiled code is not included.

Running the "make" command in the root directory will compile the code,
and it will be output as the file 'os'.

Run the code by "./os test2a"
Possible inputs: test2a, test2b, test2c, test2d, test2e, test2f, test2h
	test2i

ITEMS TO TRACE:

Each of the *.OUT files include the outputs from each of the tests.

Based on the table provided for specific output requirements, the tests
will include scheduler printouts, fault printouts, and memory printouts.

If "limited", the printouts will be granular in nature (simular to test.c)

Test2h (LRU Approx) will have ~20% less page faults than Test2f (FIFO) and
Test2i (Random).

GUIDE TO PHASE2 CODE:

base.c
	CONTAINS FAULT and INTERRUPT Handlers
	os_switch_context_complete
	os_in_it
	SVC
	EVENT_HANDLER
	And relevant helper functions
paging.c
	PAGE FAULT HANDLING
	DISK READ AND WRITE
	FAME MANAGER
	SHADOW TABLE MGMT
	And relevant helper functions
debug.c
	Printouts
*.OUT
	Prints for each test run
userdef.h
	Contains all developed definitions and functions.

