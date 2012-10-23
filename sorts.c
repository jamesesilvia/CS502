/*
*	This file contains the routines to sort the Ready and
*	Timer Queue. 
*
*	The Ready Queue is sorted based on the priority,
*	while the Timer Queue is sorted based on the wakeUp
*	Time (p_time)
*/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

//Sort Timer Queue until the queue is correctly ordered
void timer_sort ( void ){
	PCB_t * ptrPrev = NULL;
	PCB_t * ptrCurr = NULL;
	INT32 swap = 1;
	
	while (swap){
		swap = 0;
		ptrPrev = timerList;
		ptrCurr = ptrPrev->next;
		while (ptrCurr != NULL){
			if (ptrPrev->p_time > ptrCurr->p_time){
				timerSwap(ptrPrev, ptrCurr);
				ptrPrev = ptrCurr;
				ptrCurr = ptrCurr->next;
				swap = 1;
			}
			else{
				ptrPrev = ptrCurr;
				ptrCurr = ptrCurr->next;
			}
		}	
	}
}
//Swap two nodes in the linked list
void timerSwap( PCB_t *Prev, PCB_t *Curr ){
	PCB_t *temp;
	
	temp = Prev->prev;	
	//First
	if( temp == NULL ){
		Curr->prev = NULL;
		timerList = Curr;
	}
	else{
		temp->next = Curr;
		Curr->prev = temp;	
	}
	//Second
	temp = Curr->next;
	if( temp == NULL ){
		Prev->next = NULL;
	}
	else{
		temp->prev = Prev;
		Prev->next = temp;	
	}
	//Last
	Prev->prev = Curr;
	Curr->next = Prev;
	
	return;
}
//Sort Ready Queue until the queue is correctly ordered
void ready_sort ( void ){
	PCB_t * ptrPrev = NULL;
	PCB_t * ptrCurr = NULL;
	INT32 swap = 1;
	
	while (swap){
		swap = 0;
		ptrPrev = pidList;
		ptrCurr = ptrPrev->next;
		while (ptrCurr != NULL){
			if (ptrPrev->p_priority < ptrCurr->p_priority){
				readySwap(ptrPrev, ptrCurr);
				ptrPrev = ptrCurr;
				ptrCurr = ptrCurr->next;
				swap = 1;
			}
			else{
				ptrPrev = ptrCurr;
				ptrCurr = ptrCurr->next;
			}
		}	
	}
}
//Swap two nodes in the linked list
void readySwap( PCB_t *Prev, PCB_t *Curr ){
	PCB_t *temp;
	
	temp = Prev->prev;	
	//First
	if( temp == NULL ){
		Curr->prev = NULL;
		pidList = Curr;
	}
	else{
		temp->next = Curr;
		Curr->prev = temp;	
	}
	//Second
	temp = Curr->next;
	if( temp == NULL ){
		Prev->next = NULL;
	}
	else{
		temp->prev = Prev;
		Prev->next = temp;	
	}
	//Last
	Prev->prev = Curr;
	Curr->next = Prev;
	
	return;
}

