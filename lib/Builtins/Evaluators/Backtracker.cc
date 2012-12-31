/* from <http://homepage.mac.com/sigfpe/Computing/continuations.html> */
#include "Evaluators/Backtracker"
#include <setjmp.h>
#include <malloc.h>
#include <stdlib.h>

namespace Evaluators {

long *pbos;
void set_stack_beginning(long* value) {
	pbos = value;
}
/*
 * Continuation datastructure.
 */
typedef struct cont {
	jmp_buf registers;
	int n;
	long *stack;
	/*
	 * Pointer to next continuation in chain.
	 */
	struct cont *next;
} cont;

void save_stack(cont *c,long *pbos,long *ptos) {
	int n = pbos-ptos;
	int i;

	c->stack = (long *)malloc(n*sizeof(long));
	c->n = n;
	for (i = 0; i<n; ++i) {
		c->stack[i] = pbos[-i];
	}
}

cont *getcontext() {
	cont *c = (cont *)malloc(sizeof(cont));
	long tos;
	/*
	 * Save registers
	 */
	if (!setjmp(c->registers)) {
		/*
		 * Save stack
		 */
		save_stack(c,pbos,&tos);

		return c;
	} else {
		return 0;
	}
}

cont *gcont = 0;

/*
 * Save current continuation.
 */
int Backtracker_save_context(void) {
	cont *c = getcontext();
	if (c) {
		c->next = gcont;
		gcont = c;
		return 0;
	} else {
		return 1;
	}
}

void restore_stack(cont *c,int once_more) {
	volatile long padding[12];
	long tos;
	int i,n;
	/*
	 * Make sure there's enough room on the stack
	 */
	if (pbos-c->n<&tos) {
		restore_stack(c,1);
	}

	if (once_more) {
		restore_stack(c,0);
	}

	/*
	 * Copy stack back out from continuation
	 */
	n = c->n;
	for (i = 0; i<n; ++i) {
		pbos[-i] = c->stack[i];
	}
	longjmp(c->registers,1);
}

void exec_context(cont *c) {
	/*
	 * Restore stack
	 */
	restore_stack(c,1);
}

/*
 * Restore last continuation.
 */
void Backtracker_restore_context(void) {
	if (gcont) {
		cont *old = gcont;
		gcont = old->next;
		exec_context(old);
	} else {
		exit(1);
	}
}

};
