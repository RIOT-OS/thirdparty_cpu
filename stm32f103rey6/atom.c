/*
 * atom.c
 *
 *  Created on: 10.08.2012
 *      Author: pfeiffer
 */
#include "sched.h"
#include "cpu.h"

extern void sched_task_exit(void);
void sched_task_return(void);

#define STACK_MARKER 0X77777777

unsigned int atomic_set_return(unsigned int* p, unsigned int uiVal) {
	dINT();
	unsigned int uiOldVal = *p;
	*p = uiVal;
	eINT();
	return uiOldVal;
}

void cpu_switch_context_exit(void){
	asm("bl sched_run");

  /* load pdc->stackpointer in r0 */
	asm("ldr     r0, =active_thread");    /* r0 = &active_thread */
	asm("ldr     r0, [r0]");              /* r0 = *r0 = active_thread */
	asm("ldr     sp, [r0]");              /* sp = r0  restore stack pointer*/

	asm("pop		{r4}"); /* skip exception return */

  // enforce lsb bit of pc to be 1,
  // This value is stored in the T flag of xPSR when the pop pc is executed,
  // and we need to have that set to 1.
  asm("add    r0, sp, #56");
	asm("ldr    r1, [r0]");
	asm("orr    r1, #1");
	asm("str    r1, [r0]");

	asm("pop		{r4-r11}");
	asm("pop		{r0-r3,r12,lr}");   /* simulate register restor from stack */
	asm("pop		{pc}");
}

void thread_yield(void) {
	asm("svc 0x01\n");
}

__attribute__((naked))
void SVC_Handler(void)
{
	save_context();
	asm("bl sched_run");
	restore_context();
}

char * thread_stack_init(void * task_func, void * stack_start, int stack_size ) {
	unsigned int * stk;
	stk = (unsigned int *) (stack_start + stack_size);

	// marker
	stk--;
	*stk = STACK_MARKER;

	// xPSR
	stk--;
	*stk = (unsigned int) 0x21000000;

	// program counter
	stk--;
	*stk = (unsigned int) task_func;

	// link register
	stk--;
  *stk = (unsigned int) sched_task_exit;
	
	// r12
	stk--;
	*stk = (unsigned int) 0;

	// r0 - r3
	for (int i = 3; i >= 0; i--) {
		stk--;
		*stk = i;
	}

	// r11 - r4
	for (int i = 11; i >= 4; i--) {
		stk--;
		*stk = i;
	}

	// lr means exception return code 
	stk--;
	*stk = (unsigned int) 0xfffffff9; // return to Thread mode, main stack pointer

	return (char*) stk;
}

