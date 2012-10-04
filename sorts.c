#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

#include			"userdefs.h"

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

