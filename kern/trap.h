/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];
extern struct Pseudodesc idt_pd;

extern void trap_init(void);
extern void trap_init_percpu(void);
extern void print_regs(struct PushRegs *regs);
extern void print_trapframe(struct Trapframe *tf);
extern void page_fault_handler(struct Trapframe *);
extern void backtrace(struct Trapframe *);
extern void divide_zero();
extern void debug();
extern void non_mask_interrupt();
extern void breakp();
extern void overflow();
extern void bound_range_exceeded();
extern void invalid_opcode();
extern void device_not_avail();
extern void double_fault();
//extern void coprocessor_segment_overrun();
extern void invalid_TSS();
extern void segment_not_present();
extern void stack_fault();
extern void general_protection();
extern void page_fault();
//extern void unknown_trap();
extern void floating_point_error();
extern void alignment_check();
extern void machine_check();
extern void SIMD_floating_point_exception();

extern void syscall_exception();
#endif /* JOS_KERN_TRAP_H */
