// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	
//uint64_t utf_fault_va;  
	pte_t pte = vpt[VPN(addr)];
	
	if(!(err & FEC_WR)){
		panic("Denied write:%0x %e",(uint64_t)addr,err);
	}
	if(!(pte & PTE_COW)){
		panic("Denied COW: %e",err);
	}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	//sys_page_alloc

	
	if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
                panic("sys_page_alloc: %e", r);

	void *tmp_va=(void *)ROUNDDOWN(addr,PGSIZE);
        memmove(PFTEMP, tmp_va, PGSIZE);


        if ((r = sys_page_map(0,PFTEMP, 0, tmp_va, PTE_P|PTE_U|PTE_W)) < 0)
                panic("sys_page_map: %e", r);



}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{

	int r;

	// LAB 4: Your code here.
	uint64_t va = pn << PTXSHIFT;
	if(!(vpt[pn] & PTE_P))
		return -E_INVAL;

	if (va > UTOP ){
		panic("above utop\n");
	}
	
	if((vpt[pn] & PTE_W) || (vpt[pn] & PTE_COW)){	
		if((r = sys_page_map(0, (void*)va, envid, (void*)va, PTE_P | PTE_U | PTE_COW ))<0)
			panic("sys_page_map error : %e\n",r);
			
		if((r=sys_page_map(0,(void*)va,thisenv->env_id,(void*)va,PTE_P | PTE_U | PTE_COW))<0)
			panic("sys_page_map error : %e\n",r);

	}else {
		if ((r = sys_page_map(0, (void *)va, envid, (void*)va, PGOFF(vpt[pn])| PTE_COW | PTE_P | PTE_U )) < 0){
		panic("sys page map error: %e\n",r );
	}
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	int r;
	envid_t envid;
	set_pgfault_handler(pgfault);
	if((envid = sys_exofork())<0)
		panic("sys_exofork error: %e\n",envid);

	if(envid == 0){
		cprintf("I am child\n");
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}	
	
	uint8_t *va ,*stack;
	extern unsigned char end[];
	for (va = (uint8_t *)UTEXT ; va < (uint8_t *)end; va += PGSIZE ){
		if (vpt[VPN(va)] != 0){
			duppage(envid, (uint64_t)va / PGSIZE);
		}	
	}

	// copy the stack
	duppage(envid, (uint64_t)(USTACKTOP - PGSIZE) / PGSIZE);
	
	#if 0
	int i , j, pn, pn_uxstacktop = (UXSTACKTOP - PGSIZE) / PGSIZE;
	for (i = 0; i < PDX(UTOP); i++ ){
		if ((vpd[i] & PTE_P )){ // pgdir entry present?i
			for (j = 0; j < NPTENTRIES; j++ ){
				pn = i * NPTENTRIES + j;
				if (pn != pn_uxstacktop){
					duppage(envid, pn);
				}else {
					cprintf("hello world");
				}
				
			}
		}
	}
	panic("jhj");
	for (pn=0;pn<VPN(UTOP);pn++){
		
   		cprintf("(VPN(UXSTACKTOP-PGSIZE):%0x\n",(VPN(UXSTACKTOP-PGSIZE)));
//		cprintf("%0x\n", pn <<PTXSHIFT);
		if((pn!=(VPN(UXSTACKTOP-PGSIZE) ))&(vpt[pn] & PTE_P)){
			 duppage(envid,pn);
			panic("after duppage");
		}
		
		cprintf("\nentered loop to map \n");
		cprintf("%x\n", pn <<PTXSHIFT);

		if((vpd[pn]&PTE_P)&if((pn!=(VPN(UXSTACKTOP-PGSIZE) ))){
			if(vpt[pn]&PTE_W ||vpt[pn]&PTE_COW ){
				cprintf("%x\n", pn <<PTXSHIFT);
				duppage(envid,pn);
			}
			else{
				 va = pn <<PTXSHIFT ;
				cprintf("%x\n",va);
				 if((r=sys_page_map(0,(void*)va,envid,(void*)va,PGOFF(vpt[pn])))<0)
		                        panic("sys_page_map error : %e\n",r);
			}
		}		
		#endif


	if((r = sys_page_alloc(envid,(void*)(UXSTACKTOP - PGSIZE), PTE_P | PTE_W | PTE_U ))<0)	
		panic("sys_page_alloc error: %e\n",r);

	if((r = sys_env_set_status(envid, ENV_RUNNABLE))<0)
		panic("sys_env_set_status error: %e\n",r);

	if((r=sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)))
		panic("sys_env_set_pgfault_upcall error: %e\n",r);

	return envid;

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
