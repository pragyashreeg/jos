#include <inc/stab.h>

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>
#include <kern/lkm.h>

#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/ksyms.h>

#define MODTEMP 0xF0F0000/*temp address for module load*/


struct Secthdr *sht;
struct Symhdr *symtab ;
char *str_tab, *sht_strtab;
int num_sym, num_sht;


int elf_lookup_symbol(char *symbol, uint64_t *address_store, struct Elf *elf);
int fix_ksymbols();
int print_symboltable(void);
int fix_symbols();
int print_sht(void);
struct Module create_module(char* loadAddress);
int apply_relocation(uint64_t *loadaddress,struct Elf *elf);
int load_program(uint64_t load_address);

int
load_module(char *buffer, int npages/*no of pages*/){

	print_ksyms();
	return 0;

	struct Page *page = NULL;
	int perm = PTE_P | PTE_W;
	struct Elf *elf = (struct Elf *)buffer;
	void (*ptr_init_module)();

	//TODO : Sanity check.
	if (elf->e_magic == ELF_MAGIC){
		cprintf("###### ...In LKM.Its an ELF buffer..######\n");
	}else
		return -1;


	/****************************************************CREATE CONV VARIABLES *******************************************/

	// create convenient variables.
	// SHT, symbol tabs, string tabs
	int i;
	struct Secthdr *sym_tab_entry, *str_tab_entry, *sht_strtab_entry;

	//section header
	sht = (struct Secthdr *)((char *)elf + elf->e_shoff);
	num_sht = elf->e_shnum;

	// string table for the SHT.
	sht_strtab_entry = &sht[elf->e_shstrndx];
	sht_strtab = (char *)((char *)elf + sht_strtab_entry->sh_offset);

	for(i = 0; i < num_sht ; i++){
		if (sht[i].sh_type == ELF_SHT_SYMTAB){
			// symbol table
			sym_tab_entry = sht + i;
			symtab = (struct Symhdr *)((char *)elf + sym_tab_entry->sh_offset);
			num_sym = sym_tab_entry->sh_size / sizeof(struct Symhdr);
			//cprintf("num sym %d\n", num_sym);


		}else if (sht[i].sh_type == ELF_SHT_STRTAB && (sht + i) != sht_strtab_entry){
			//string table for symbol
			str_tab_entry = sht + i;
			str_tab = (char *)(str_tab_entry->sh_offset + (char *)elf);
		}
	}


	/************************************ALLOCATE LOCATION IN MEMORY FOR THE MODULE********************************************************/

	//find the size of the lodable modules
	size_t size = 0;
	for (i = 0; i < num_sht; i++){
		if (sht[i].sh_type == ELF_SHT_PROGBITS && sht[i].sh_flags & ELF_SHF_ALLOC){
			size += sht[i].sh_size;
		}

	}
	//cprintf("size of the module %x\n", size);
	char loadAddress[size];
	memset((void *)loadAddress, 0, size);

	/*uint64_t *loadAddress = (uint64_t *)MODTEMP;
	//memove to kernel space
	page = page_alloc(ALLOC_ZERO);
	if (!page){
		panic("no page");
		return -E_NO_MEM;
	}

	if ((page_insert(curenv->env_pml4e, page, loadAddress, perm )) != 0 ){
		// free the page just created.
		page_free(page);
		return -E_NO_MEM;
	}*/

	//copy in memory
	uint64_t sect_start_addr = (uint64_t)loadAddress;
	for (i = 0; i < num_sht; i++){
		if (sht[i].sh_type == ELF_SHT_PROGBITS && sht[i].sh_flags & ELF_SHF_ALLOC){
			size = sht[i].sh_size;
			char *name = (char *)(sht_strtab + sht[i].sh_name);
			//cprintf("type : %s, size :%d, elf %x\n",name, size, elf);
			uint64_t src =(uint64_t)(sht[i].sh_offset + (char *)elf);
			(sht + i)->sh_addr =sect_start_addr; //dest
			memcpy((void *)sect_start_addr, (void *)src , size);
			//cprintf("%x\n",*(uint64_t *)src);
			//cprintf("%x\n",*(uint64_t *)sect_start_addr);
			sect_start_addr += size;
		}

	}

	//create module
	struct Module m;
	m= create_module((char *)loadAddress);
	cprintf("init : %x, deinit %x\n", m.init, m.deinit);

	/************************************FIX SYMBOLS********************************************************/
	//fix_ksymbols();
	fix_symbols();

	/*******************************************RELOCATION************************************************************/
	apply_relocation((uint64_t *)loadAddress, elf);
	//print_symboltable();
	//print_sht();

	//call init
	((void (*)(void)) (m.init))();
	/*ptr_init_module = (void (*)())(m.init);
	cprintf("%x\n", *((uint64_t *)m.init));
	(*ptr_init_module)();*/

	return 0;
}


int
apply_relocation(uint64_t *loadaddress, struct Elf *elf){
	cprintf("aply relocation \n");

	int i;
	int err,j;

	uint64_t value, *where;
	struct Symhdr *symbol;
	struct Rela *rela;
	uint64_t sh_type;


	for(i=0;i<num_sht;i++){
		uint32_t infosec = sht[i].sh_info; // the section abt which the relocation section is
		//uint32_t symtab = sht[i].sh_link; //which symbol table

		if(infosec >= num_sym){
			continue;
		}

		if(!(sht[infosec].sh_flags & ELF_SHF_ALLOC)){ //if not a memory related section
			continue;
		}

		sh_type = sht[i].sh_type;

		switch(sh_type){
		case ELF_SHT_REL:
			//without addend
			cprintf("not handling this in here\n");

			break;
		case ELF_SHT_RELA:

			rela = (void *)(sht[i].sh_offset + (char *)elf);
			for (j=0; j < sht[i].sh_size / (sizeof (struct Rela)); j++){
				//where = (void *)((char *)loadaddress + rela[j].r_offset);
				where = (void *)((char *)(sht[infosec].sh_addr) + rela[j].r_offset);

				symbol =(void *)(symtab + ELF_R_SYM(rela[j].r_info));
				value = symbol->st_value + rela[j].r_addend ;
				//cprintf("whre %x, value %x\n", where, value);

				switch(ELF_R_TYPE(rela[j].r_info)){
				case R_X86_64_64:
					//cprintf("R_X86_64_64\n");
					*(uint64_t *)where = value;
					break;
				case R_X86_64_32:
					//cprintf("R_X86_64_32\n");
					*(uint32_t *)where = value;
					break;
				case R_X86_64_PC32:
					//cprintf("R_X86_64_32\n");
					value -= (uint64_t)where;
					*(uint32_t *)where = value;
					break;
				}
			}
			break;
		default:
			continue;
		}

	}
	return 0;
}


struct Module
create_module(char* loadAddress){
	struct Module mod;
	int i;
	mod.init = 0;
	mod.deinit = 0;
	//look up init and cleanup
	for(i=0; i<num_sym;i++){
		char *name = (char *)(str_tab + symtab[i].st_name);
		if (!(strcmp(name, "init_module"))){
			cprintf("init module found\n");
			mod.init =(uint64_t *)(loadAddress + symtab[i].st_value);
		}else if(!(strcmp(name, "clean_module"))){
			cprintf("clean_module found\n");
			mod.deinit = (uint64_t *)(loadAddress + symtab[i].st_value);
		}
	}
	return mod;
}

int
print_symboltable(void){
	cprintf("Symbols...\n");
	int i;
	for (i = 0; i < num_sym; i++){
		char *name = (char *)(str_tab + symtab[i].st_name);
		cprintf("name : %s, value : %x\n", name, symtab[i].st_value);
	}

	return 0;
}

int
print_sht(void){
	cprintf("sections....\n");
	int i;
	for (i =0 ; i <num_sht; i++ ){
		char *name = (char *)(sht_strtab + sht[i].sh_name);
		cprintf("name : %s, type :%x,addr: %x\n", name, sht[i].sh_type, sht[i].sh_addr);
	}
	return 0;
}



int
fix_ksymbols(){
	int err, i;

	for (i = 1; i < num_sym; i++){

		if (symtab[i].st_shndx == SHN_UNDEF){
			char *name = (char *)(str_tab + symtab[i].st_name);

			//fetch kernel value
			struct Ksymbol symbol_store;
			if ((err = get_ksyms(name, &symbol_store)) < 0){
				cprintf("could not find the symbol\n");
				return -1;
			}

			(symtab + i)->st_value =(uint64_t) symbol_store.value;
		}
	}

	return 0;

}

int
fix_symbols(){
	cprintf("fix symbols\n");
	int err, i;

	for (i = 1; i < num_sym; i++){

		if (symtab[i].st_shndx == SHN_UNDEF){
			char *name = (char *)(str_tab + symtab[i].st_name);

			//fetch kernel value
			struct Ksymbol symbol_store;
			if ((err = get_ksyms(name, &symbol_store)) < 0){
				cprintf("could not find the symbol\n");
				return -1;
			}

			(symtab + i)->st_value =(uint64_t) symbol_store.value;
		}else { //local variables
			//local vars
			uint64_t infosec = symtab[i].st_shndx;
			//commit the value
			(symtab + i)->st_value += sht[infosec].sh_addr;
		}
	}

	return 0;

}



#if 0

int
fix_symbols(struct Elf *elf){
	char * baseAddr = (char *)elf;
	int i;

	cprintf("fixing symbol table of elf at %x\n", baseAddr);
	for (i = 0; i < num_sym; i++){
		if (symtab[i].st_shndx == SHN_ABS){
			continue;
		}

		if (symtab[i].st_shndx == SHN_UNDEF){
			//kernel symbols
			char *name = (char *)((str_tab_entry->sh_offset) + symtab[i].st_name + baseAddr);
			if (fix_ksymbols(name, elf) < 0){
				return -1;
			}

		}else {
			//local vars
			uint64_t infosec = symtab[i].st_shndx;
			uint64_t value = sht[infosec].sh_offset + baseAddr;
			//commit the value
			(symtab + i)->st_value = value;

		}
		return 0;
	}



	int unload_module(char *buffer){
		cprintf("unload in lkm\n");
		return 0;
	}
	/*
int
apply_relocation(struct Elf *elf){
	char* baseAddr = (char *)elf;
	cprintf("Fixing symbols at address %x\n", elf);

	uint32_t sh_type;
	void *where;
	struct Symtab *symbol;

	//relocation
	for(i=1 ; i < elf->e_shnum; i++){

		int err;
		uint32_t infosec = sht[i].sh_info; // the section abt which the relocation section is
		uint32_t symtab = sht[i].sh_link; //symbol table index
		uint64_t value;
		if(infosec >= elf->e_shnum){
			continue;
		}

		if(!(sht[infosec].sh_flags & ELF_SHF_ALLOC)){ //if not a memory related section
			continue;
		}

		//find relocation types
		sh_type = sht[i].sh_type;

		switch(sh_type){
		case ELF_SHT_REL:
			//without addend
			cprintf("not handling this in here\n");

			break;
		case ELF_SHT_RELA:
			//with addend
			rela = (void *)(sht[i].sh_offset + baseAddr);
			//cprintf("address : %x, rela offset : %x, info %x\n",sh[i].sh_offset, rela->r_offset, rela->r_info);
			//cprintf("size :%d, size rela %d\n", sh[i].sh_size,(sizeof (*rela)) );
			//iterate thru all the rela entries
			for (j=0; j < sht[i].sh_size / (sizeof (*rela)); j++){
				cprintf("j %d, offset %x\n", j, rela[j].r_offset);
				where = (void *)(sht[infosec].sh_offset + baseAddr + rela[j].r_offset);
				symbol = (void *)(sht[symtab].sh_offset + ELF_R_SYM(rela[j].r_info));
				value = symbol->st_value + rela[j].r_addend +(uint64_t)baseAddr; //resolving it here
				cprintf("whre %x, value %x\n", where, value);

				switch(ELF_R_TYPE(rela[j].r_info)){
				case R_X86_64_64:
	 *(uint64_t *)where = value;
					break;
				case R_X86_64_32:
	 *(uint32_t *)where = value;
					break;
				case R_X86_64_PC32:
					value -= (uint64_t)where;
	 *(uint32_t *)where = value;
				}
			}
			break;
		}
	}


}*/

	int
	elf_lookup_symbol(char *symbol, uint64_t *address_store,struct Elf *elf){
		int i;
		char * baseAddr = (char *) elf;
		cprintf("looking for symbol %s in elf %x\n", symbol, baseAddr);
		if (elf->e_magic != ELF_MAGIC){
			cprintf("Lookup failed..Its not an ELF buffer.\n");
			return -1;
		}

		//find symbol in symb table
		for (i = 0; i < num_sym; i ++){
			char *name = (char *)((str_tab_entry->sh_offset) + symtab[i].st_name + baseAddr);
			//cprintf("name : %s\n", name);
			if(!strcmp(name, symbol)){
				cprintf("symbol found\n");
				*address_store = symtab[i].st_value;
				return 0;
			}
		}

		//find in the text

		return -1;
	}




#if 0

	struct Elf *elf = (struct Elf *)buffer;
	/*shstrtab is the SHT entry corresponding to section header string table*/
	/*strtab is the SHT entry corresponding to the Symbol staring table*/
	struct Secthdr *sh, *esh, *shstrtab, *strtab, *symtab;
	//place holder of the init module function
	void (*ptr_init_module)();
	struct Symtab *sym, *esym;
	struct	Rela *rela;
	struct Rel *rel;
	int i,j;



	if (elf->e_magic == ELF_MAGIC){
		cprintf("###### ...In LKM.Its an ELF buffer..######\n");
		//cprintf("section name str table %d\n", elf->e_shstrndx);
		// parse elf

		sh = (struct Secthdr *)(buffer + elf->e_shoff);
		esh = sh + elf->e_shnum;

		//initialize the SHT strtable
		shstrtab = &sh[elf->e_shstrndx];

		//cprintf("elf sct header count : %d, sizeof Secthdr: %x, sh start : %x, sh end : %x\n", elf->e_shnum,sizeof(struct Secthdr), sh, esh);
		//cprintf("iterating thru the sht \n....");
		for(; sh < esh ; sh++){
			cprintf("name, %d,type %d, addr %x, link %d, info %d\n",sh->sh_name, sh->sh_type, sh->sh_addr, sh->sh_link, sh->sh_info );
			if (sh->sh_type == ELF_SHT_SYMTAB){
				symtab = sh;
			}else if (sh->sh_type == ELF_SHT_STRTAB && sh!=shstrtab)
				strtab = sh;
			//cprintf("dump : %x\n", sh);
			//cprintf("type : %d\n", sh->sh_type);
		}
		//reset sh
		sh = (struct Secthdr *)(buffer + elf->e_shoff);



		//cprintf("we are reading sh of type %d and its size is %d. sizeof sym :%d\n", symtab->sh_type, symtab->sh_size, sizeof(struct Symtab));
		//cprintf("test string table: %s\n", (buffer + shstrtab->sh_offset)[shstrtab->sh_name]);
		//cprintf("test string table: %s\n", (char *)(buffer + shstrtab->sh_offset) + shstrtab->sh_name);


		//the actual table
		sym = (struct Symtab *)(buffer + symtab->sh_offset);
		esym = sym + (symtab->sh_size / sizeof(struct Symtab) );

		//fix symbols
		for (;sym < esym ; sym++){
			//cprintf("name: %s, value %x \n", (char *)(buffer + strtab->sh_offset) + sym->st_name, sym->st_value);
			//wite it for cprintf
			char *name = (char *)(buffer + strtab->sh_offset) + sym->st_name;
			if(!strcmp(name, "cprintf")){
				//cprintf("name: %s, value %x \n", (char *)(buffer + strtab->sh_offset) + sym->st_name, sym->st_value);
				//cprintf("old value: %x\n",sym->st_value );
				// add the adddress of cprintf
				//fix up symbols
				sym->st_value = (uint64_t)&cprintf;
				/*ptr_init_module = (void(*)())(sym->st_value);
				(*ptr_init_module)("new value: %x",sym->st_value );*/
				//  *ptr_init_module();
				assert(sym->st_value!=0);
			}else if(!strcmp(name, "init_module")){
				//sym->st_value = (uint64_t)buffer;
			}
		}



		uint32_t sh_type;
		void *where;
		struct Symtab *symbol;

		//do relocations
		for(i=1 ; i < elf->e_shnum; i++){
			cprintf("i %d\n", i);
			int err;
			uint32_t infosec = sh[i].sh_info; // the section abt which the relocation section is
			uint32_t symtab = sh[i].sh_link; //symbol table index
			uint64_t value;
			if(infosec >= elf->e_shnum){
				continue;
			}

			if(!(sh[infosec].sh_flags & ELF_SHF_ALLOC)){ //if not a memory related section
				continue;
			}

			//find relocation types
			sh_type = sh[i].sh_type;
			switch(sh_type){
			case ELF_SHT_REL:
				//without addend
				cprintf("not handling this in here\n");

				break;
			case ELF_SHT_RELA:
				//with addend
				rela = (void *)(sh[i].sh_offset + buffer);
				//cprintf("address : %x, rela offset : %x, info %x\n",sh[i].sh_offset, rela->r_offset, rela->r_info);
				//cprintf("size :%d, size rela %d\n", sh[i].sh_size,(sizeof (*rela)) );
				//iterate thru all the rela entries
				for (j=0; j < sh[i].sh_size / (sizeof (*rela)); j++){
					cprintf("j %d, offset %x\n", j, rela[j].r_offset);
					where = (void *)(sh[infosec].sh_offset + buffer + rela[j].r_offset);
					symbol = (void *)(sh[symtab].sh_offset + buffer+  ELF_R_SYM(rela[j].r_info));
					value = symbol->st_value + rela[j].r_addend +(uint64_t)buffer; //resolving it here
					cprintf("whre %x, value %x\n", where, value);

					switch(ELF_R_TYPE(rela[j].r_info)){
					case R_X86_64_64:
						*(uint64_t *)where = value;
						break;
					case R_X86_64_32:
						*(uint32_t *)where = value;
						break;
					case R_X86_64_PC32:
						value -= (uint64_t)where;
						*(uint32_t *)where = value;
					}
				}
				break;
			}
		}
		/*
		//reset the sym table
		sym = (struct Symtab *)(buffer + symtab->sh_offset);
		//print symbol table
		for (;sym < esym ; sym++){
			cprintf("name: %s, value %x \n", (char *)(buffer + strtab->sh_offset) + sym->st_name, sym->st_value);
			if(!strcmp(name, "init_module")){
				ptr_init_module = (void (*)())(sym->st_value);
				cprintf("calling function at address \n" );
				(*ptr_init_module)();
			}
		}*/

		//uint64_t initmod = 0xef7fd380;
		ptr_init_module = (void (*)())(0xef7fd380);
		//(*ptr_init_module)();
		cprintf("address %x\n", *ptr_init_module);
		//ptr_init_module();
		//
		//cprintf("%x", sh[12].sh_type);

		// read the symbol table , find the cprintf << using hack for now.

		// call init of the program*/

	}
#endif


#endif



