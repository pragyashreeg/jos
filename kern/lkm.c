#include <inc/stab.h>

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>


extern const struct Stab __STAB_BEGIN__[];	// Beginning of stabs table
extern const struct Stab __STAB_END__[];	// End of stabs table
extern const char __STABSTR_BEGIN__[];		// Beginning of string table
extern const char __STABSTR_END__[];		// End of string table



int
load_module(char *buffer){

	/*const struct Stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;

	stabs = __STAB_BEGIN__;
	stab_end = __STAB_END__;
	stabstr = __STABSTR_BEGIN__;
	stabstr_end = __STABSTR_END__;*/

	/*// String table validity checks
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0){
		cprintf("invalid symbol table\n");
		return -1;
	}
*/

	struct Elf *elf = (struct Elf *)buffer;
	struct Secthdr *sh, *esh;
	struct Symtab *sym;
	if (elf->e_magic == ELF_MAGIC){
		cprintf("###### ...In LKM.Its an ELF buffer..######\n");
		// parse elf

		sh = (struct Secthdr *)(buffer + elf->e_shoff);
		esh = sh + elf->e_shnum;
		cprintf("elf sct header count : %d, sizeof Secthdr: %x, sh start : %x, sh end : %x\n", elf->e_shnum,sizeof(struct Secthdr), sh, esh);
		for(; sh < esh ; sh++){
			if (sh->sh_type == ELF_SHT_SYMTAB){
				cprintf("symbol table found\n");
				break;
			}
			//cprintf("dump : %x\n", sh);
			cprintf("type : %d\n", sh->sh_type);
		}
		cprintf("we are reading sh of type %x and its size is %x", sh->sh_type, sh->sh_size);
		sym = (struct Symtab *)(buffer + sh->sh_offset);

		//cprintf("%x", sh[12].sh_type);

		// read the symbol table , find the cprintf



		// add the adddress of cprintf
		// call init of the program

	}


	return 0;
}

int unload_module(char *buffer){
	cprintf("unload in lkm\n");
	return 0;
}
