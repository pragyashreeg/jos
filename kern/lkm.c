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

#define MODTEMP 0xF050000/*modules are loaded in here. 10 pages*/
#define NLKM 10 /*support only 10 LKMs*/

//module list
struct Module *modules = NULL;

//current ELF variables
struct Secthdr *sht;
struct Symhdr *symtab ;
char *str_tab, *sht_strtab;
int num_sym, num_sht;
struct Elf *elf;

//called from kern/init
void
lkm_init(void){

	int c;
	//inited in pmap.c
	if(modules == NULL){
		panic("modules have not been initialized yet\n");
	}
	//
	for (c=0; c < NLKM -1; c++){
		modules[c].init = 0;
		modules[c].deinit = 0;
		/*modules[c].version = "";
		modules[c].name= "";*/
		modules[c].loadAddress = (uint64_t *)(MODTEMP + c*(uint64_t)PGSIZE);
		modules[c].isLoaded = false; //initially everything is free
	}

}

//helper/debug function
int
print_modules(int onlyLoaded){
	int c;
	cprintf("printing module list..\n");
	for(c=0; c <NLKM-1; c++){
		if (!onlyLoaded || modules[c].isLoaded)
			cprintf("name : %s , init : %x, deinit : %x, version = %d, load Address : %x\n ",modules[c].name, modules[c].init,modules[c].deinit, modules[c].version, modules[c].loadAddress);
	}
	return 0;
}

int
list_module(){
	int find = false;
	int c;
	cprintf("Listing the modules currently installed..\n");
	for(c=0; c <NLKM-1; c++){
		if (modules[c].isLoaded){
			find = true;
			cprintf("Module : %s(version = %s) is loaded at kernel load Address : %x\n",modules[c].name, modules[c].version, modules[c].loadAddress);
		}

	}
	if (!find){
		cprintf(".....No modules installed \n");
	}
	return 0;
}

/*This is the crux of the LKM*/
int
load_module(char *buffer, char *path){

	//print_symList();

	struct Page *page = NULL;
	int perm = PTE_P | PTE_W ;
	elf = (struct Elf *)buffer;
	void (*ptr_init_module)();
	int err, c;
	int upgrade = -1;

	//TODO : Sanity check.
	if (elf->e_magic == ELF_MAGIC){
		cprintf("*******************\n");
		cprintf("installing module %s\n", path);
	}else{
		cprintf("Failed to load module Not a valid ELF");
		return -E_BAD_ELF;
	}

	//jos limitation patch
	if (!strncmp("hellomodv", path, 9)){
		strcpy(path, "hellomodv");
	}

	//check if module already loaded
	if (((err = lookup_module(path)) >= 0)){ //patch to overcome JOS limitation
		upgrade = err; // the old module
		//return -E_LKM_FAIL;

	}
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

	//check if an old module. Just a version change

	/************************************ALLOCATE LOCATION IN MEMORY FOR THE MODULE********************************************************/
	if((err = create_Module()) < 0){
		cprintf("failed to create modules %e", err);
		return err;
	}
	c = err; // its a module
	cprintf("module %d created\n",c);


	load_program(c);
	//cprintf("init : %x, deinit %x\n", modules[c].init, modules[c].deinit);

	/************************************FIX SYMBOLS********************************************************/
	//fix_ksymbols();
	if (fix_symbols() < 0){
		cprintf("failed to load module. could not resolve references\n");
		return -E_BAD_SYMBOL;
	}

	/*******************************************RELOCATION************************************************************/
	apply_relocation((uint64_t *)modules[c].loadAddress, elf);

	/***FIX THE METADATA*****/
	get_meta(c);
	//fix the name here.
	strncpy(modules[c].name, path, MOD_NAME_LEN);//name is the name of the file
	//check if upgrade
	if (upgrade >= 0){ //possibly an upgrade
		cprintf("module already loaded..checking version\n");
		if(!strcmp(modules[upgrade].version, modules[c].version )){
			//not an upgrade.
			//clear the module
			cprintf("its the same old boring version\n... exiting\n");
			remove_module(c);
			return E_LKM_FAIL;
		}else {//an upgrade.
			cprintf("upgrade found\n");
			cprintf("upgrading module %s, from %s to %s\n", modules[c].name, modules[upgrade].version, modules[c].version);
			remove_module(upgrade);
		}
	}

	//call init

	lcr3(boot_cr3);
	((void (*)(void)) (modules[c].init))();
	lcr3(curenv->env_cr3);
	//print_symList();
	return 0;
}

int
load_program(int c){
	size_t size;
	int i;
	if (!modules[c].isLoaded || modules[c].loadAddress == NULL){
		return -E_LKM_FAIL;
	}

	//copy in the actual location
	uint64_t sect_start_addr = (uint64_t)modules[c].loadAddress;
	pte_t *pte_store;
	struct Page *pg = page_lookup(boot_pml4e, (void *)MODTEMP,&pte_store);
	for (i = 0; i < num_sht; i++){
		if (sht[i].sh_type == ELF_SHT_PROGBITS && sht[i].sh_flags & ELF_SHF_ALLOC){
			size = sht[i].sh_size;
			char *name = (char *)(sht_strtab + sht[i].sh_name);
			//cprintf("type : %s, size :%d, elf %x\n",name, size, elf);
			uint64_t src =(uint64_t)(sht[i].sh_offset + (char *)elf);
			(sht + i)->sh_addr =sect_start_addr; //dest
			memcpy((void *)sect_start_addr, (void *)src , size);
			sect_start_addr += size;
		}

	}
	return 0;
}

int
apply_relocation(uint64_t *loadaddress, struct Elf *elf){
	//cprintf("aply relocation \n");

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

/*adds a module to the module list
 * and returns the index to the array*/
int
create_Module(){
	//find a free module.
	int c, err, i;
	for(c=0; c<NLKM; c++){
		if(!modules[c].isLoaded){
			break; //found a free module
		}
	}
	if (c == NLKM)
		return -NO_FREE_MODULE;

	modules[c].init = 0;
	modules[c].deinit = 0;
	//allocate space in k-memory.
	if ((err = allocate_memory(c)) < 0){
		return err;
	}

	//create a module at module[i]
	//look up init and cleanup
	for(i=0; i<num_sym;i++){
		char *name = (char *)(str_tab + symtab[i].st_name);
		if (!(strcmp(name, "init_module"))){
			//cprintf("init module found\n");
			modules[c].init =(uint64_t *)((char *)(modules[c].loadAddress) + symtab[i].st_value);
		}else if(!(strcmp(name, "clean_module"))){
			//cprintf("clean_module found\n");
			modules[c].deinit = (uint64_t *)( (char *)(modules[c].loadAddress) + symtab[i].st_value);
		}
	}
	if (modules[c].init ==0 || modules[c].deinit == 0){
		return -E_BAD_MODULE;
	}
	modules[c].isLoaded = true; //commit module
	return c;
}

/*call this after fixing relocation
 * the version and name would have been
 * resolved by then*/
int
get_meta(int c){
	int i;
	if (c > NLKM){
		cprintf("failed to fix meta data.. invalid module\n");
		return -E_BAD_MODULE;
	}
	if (!modules[c].isLoaded){
		cprintf("failed to load module\n");
		return -E_BAD_MODULE;
	}
	for(i=0; i<num_sym;i++){
		char *name = (char *)(str_tab + symtab[i].st_name);
		if (!(strcmp(name, "version"))){
			//cprintf("version found\n");
			strcpy(modules[c].version , (char *)(*(uint64_t *)symtab[i].st_value));
			cprintf("version : %s\n",modules[c].version, modules[c].version );
		}else if(!(strcmp(name, "author"))){
			//cprintf("author found\n");
			strcpy(modules[c].author, (char *)(*(uint64_t *)symtab[i].st_value));
			cprintf("author : %s\n",modules[c].author);
		}
	}

	return 0;
}

/*loads actual physical resgion for the lkm to
 * reside. Returns 0 on success, <0 on error*/
int
allocate_memory(int c){
	int i;
	struct Page *page;
	int perm = PTE_P | PTE_W ; //TODO: change this permission

	if (c > NLKM){
		cprintf("invalid module number");
		return -E_LKM_FAIL;
	}

	//find the size of the lodable modules
	size_t size = 0;
	for (i = 0; i < num_sht; i++){
		if (sht[i].sh_type == ELF_SHT_PROGBITS && sht[i].sh_flags & ELF_SHF_ALLOC){
			size += sht[i].sh_size;
		}

	}
	// we dont support modules bigger than PGSIZE
	if (size > (uint64_t)PGSIZE){
		cprintf("too big module\n");
		return -E_LKM_FAIL;
	}

	//now allocate memory
	uint64_t *loadAddress = (uint64_t *)modules[c].loadAddress;

	//memove to kernel space
	page = page_alloc(ALLOC_ZERO);
	if (!page){
		panic("no page");
		return -E_NO_MEM;
	}
	//push in kernel.
	if ((page_insert(boot_pml4e/*curenv->env_pml4e*/, page, loadAddress, perm )) != 0 ){
		// free the page just created.
		page_free(page);
		return -E_NO_MEM;
	}
	//push in current user memory
	if ((page_insert(curenv->env_pml4e, page, loadAddress, perm )) != 0 ){
		// free the page just created.
		page_free(page);
		return -E_NO_MEM;
	}
	return 0;

}

int
free_module(){
	//find which module in the list. use name as key
	return 0;
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
	//cprintf("fix symbols\n");
	int err, i;

	for (i = 1; i < num_sym; i++){

		if (symtab[i].st_shndx == SHN_UNDEF){
			char *name = (char *)(str_tab + symtab[i].st_name);

			//fetch kernel value
			struct Ksymbol symbol_store;
			memset(&symbol_store, 0, sizeof(struct Ksymbol));
			if ((err = get_ksyms(name, &symbol_store)) < 0){
				cprintf("could not find the symbol\n");
				return -E_BAD_SYMBOL;
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

int
lookup_module(char *mname){
	int c;
	for(c=0; c<NLKM; c++){
		if (modules[c].isLoaded && !strcmp(mname, modules[c].name)){
			return c;
		}
	}
	return -E_BAD_MODULE;
}

int
remove_module(int c){
	//disallocate the page
	uint64_t *loadAddress = (uint64_t *)modules[c].loadAddress;
	page_remove(boot_pml4e,loadAddress);
	page_remove(curenv->env_pml4e,loadAddress);

	//remove entry from the module list
	modules[c].init = 0;
	modules[c].deinit = 0;
	memset(modules[c].version,0, MOD_NAME_LEN);
	memset(modules[c].name,0, MOD_NAME_LEN);
	memset(modules[c].author,0, MOD_NAME_LEN);
	modules[c].loadAddress = (uint64_t *)(MODTEMP + c*(uint64_t)PGSIZE);
	modules[c].isLoaded = false; //initially everything is free
	return 0;
}

int
unload_module(char *mname){
	int err, c;
	cprintf("******************\n\n");
	pte_t *pte_store;
	struct Page *page = page_lookup(boot_pml4e, (void *)MODTEMP,&pte_store);
	//cprintf("boot_pml4e %x, page %x \n", boot_pml4e, page2pa(page));

	//lookup the module
	if((err = lookup_module(mname)) < 0){
		cprintf("this module is not loaded\n");
		return err;
	}
	//else its module
	c = err;

	lcr3(boot_cr3);
	((void (*)(void)) (modules[c].deinit))();
	lcr3(curenv->env_cr3);
	//clean up.
	remove_module(c);

	return 0;

}
