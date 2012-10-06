/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>
#include <kern/macro.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushq $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushq $0; 							\
	pushq $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
   Find the PUSHA,POPA macros in kern/macro.h. Since amd64 doesn't support
   pusha,popa so you will find these macros handy.
 */

TRAPHANDLER_NOEC(divide_zero, T_DIVIDE);
TRAPHANDLER_NOEC(debug,T_DEBUG);
TRAPHANDLER_NOEC(non_mask_interrupt, T_NMI);
TRAPHANDLER_NOEC(breakp,T_BRKPT);
TRAPHANDLER_NOEC(overflow,T_OFLOW);
TRAPHANDLER_NOEC(bound_range_exceeded,T_BOUND);
TRAPHANDLER_NOEC(invalid_opcode, T_ILLOP);
TRAPHANDLER_NOEC(device_not_avail,T_DEVICE);
TRAPHANDLER(double_fault, T_DBLFLT);
#TRAPHANDLER_NOEC(coprocessor_segment_overrun, 9);
TRAPHANDLER(invalid_TSS, T_TSS);
TRAPHANDLER(segment_not_present, T_SEGNP);
TRAPHANDLER(stack_fault,T_STACK);
TRAPHANDLER(general_protection,T_GPFLT);
TRAPHANDLER(page_fault, T_PGFLT);

TRAPHANDLER_NOEC(syscall_exception, T_SYSCALL);

#TRAPHANDLER_NOEC(unknown_trap, 15);
TRAPHANDLER_NOEC(floating_point_error, T_FPERR);
TRAPHANDLER(alignment_check,T_ALIGN);
TRAPHANDLER_NOEC(machine_check, T_MCHK);
TRAPHANDLER_NOEC(SIMD_floating_point_exception, T_SIMDERR);


/*
 * Lab 3: Your code here for _alltraps
 */
.align 2
_alltraps:
	subq $16, %rsp
	movw %ds, 8(%rsp) 
	movw %es, 0(%rsp)
	PUSHA
	movw $GD_KD,%ax
	movw %ax, %es
	movw %ax, %ds
	pushq %rsp
	movq %rsp, %rdi
	addq $8, %rdi
	call trap
	POPA_
	lret
/*

	pushq $GD_KD
	pushq $GD_KD
	PUSHA
	movl %esp, %edi
	call trap
	POPA	
*/
