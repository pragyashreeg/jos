#include <inc/stab.h>
#include <inc/elf.h>
#include <kern/ksyms.h>
#include <inc/types.h>

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/stdio.h>

//global variable
extern const struct Stab __STAB_BEGIN__[];	// Beginning of stabs table
extern const struct Stab __STAB_END__[];	// End of stabs table
extern const char __STABSTR_BEGIN__[];		// Beginning of string table
extern const char __STABSTR_END__[];		// End of string table


struct Ksymbol symVal[MAX_SYM_NUM];
int sym_count = 0;

int
load_ksyms(void){
	//could make it more robust by adding more syms in here
	return 0;
}

/* success 0
 * else -1 */

int
get_ksyms(char *symbol_name, struct Ksymbol *symbol_store){ //TODO : ideally symbol shud be a structure in ksym.h

	const struct Stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;
	char *name;

	stabs = __STAB_BEGIN__;
	stab_end = __STAB_END__;
	stabstr = __STABSTR_BEGIN__;
	stabstr_end = __STABSTR_END__;

	// String table validity checks
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
		return -E_BAD_SYMBOL;

	int i;
	int num_syms = (stab_end - stabs);
	//memset(symbol_store, 0, sizeof(struct Ksymbol));
	for (i = 0; i < num_syms -1 ; i++){
		name= (char * )(stabstr + stabs[i].n_strx);
		if(strncmp(name,symbol_name,strlen(symbol_name))==0){
			symbol_store->value = (uint64_t)stabs[i].n_value;
			strcpy(symbol_store->name,name);
			return 0;
		}
	}

	int j;

	for(j=0; j < MAX_SYM_NUM; j++){
		name = (char *)symVal[j].name;
		if (!strcmp(name, symbol_name)){
			symbol_store->value = symVal[j].value;
			strcpy(symbol_store->name,symVal[j].name);
			return 0;
		}
	}

	return -E_BAD_SYMBOL;
}


int
print_symList(void){
	cprintf("printing sym list\n");
	int i;
	for(i=0; i<MAX_SYM_NUM; i++){
		cprintf("sym_count: %d name : %s, value: %0x\n",sym_count, symVal[i].name, symVal[i].value);
	}
	return 0;
}

int
print_ksyms(void){
	const struct Stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;
	char *name;

	stabs = __STAB_BEGIN__;
	stab_end = __STAB_END__;
	stabstr = __STABSTR_BEGIN__;
	stabstr_end = __STABSTR_END__;

	// String table validity checks
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
		return -1;

	int i;
	int num_syms = (stab_end - stabs) + 1;

	for (i = 0; i < num_syms; i++){
		name= (char * )(stabstr + stabs[i].n_strx);
		cprintf("name : %s value : %x\n", name, stabs[i].n_value);

	}
	print_symList();
	return 0;
}

int
remove_ksyms(char *symbol_name){
	int s;
	for(s=0; s<MAX_SYM_NUM; s++){
		char *name = (char *)symVal[s].name;
		if (!strcmp(name, symbol_name)){
			// found the symbol. remove.
			symVal[s].value = 0;
			symVal[s].is_valid = false;
			sym_count--;
			return 0;
		}
	}

	return -E_BAD_SYMBOL;
}

int
put_ksyms(char *name, uint64_t value){
	cprintf("in put ksyms\n");
	int s=0;
	//find a free slot
	for(s=0; s<MAX_SYM_NUM; s++){
		if(!symVal[s].is_valid){
			break;
		}
	}

	if (s >= MAX_SYM_NUM){
		return -E_BAD_SYMBOL;
	}

	//else we found a slot.
	strcpy(symVal[s].name, name);
	symVal[s].value = value;
	symVal[s].is_valid = true;
	sym_count++;
	//cprintf("sym_count: %d name : %s, value: %0x\n",sym_count, symVal[sym_count].name, symVal[sym_count].value);    sym_count ++;

	return 0;
}
