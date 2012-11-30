#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

int
load_module(char *buffer){

	struct Elf *elf = (struct Elf *)buffer;
	if (elf->e_magic == ELF_MAGIC){
		cprintf("In LKM.Its an ELF buffer\n");
		//parse elf

	}
	return 0;
}

int unload_module(char *buffer){
	cprintf("unload in lkm\n");
	return 0;
}
