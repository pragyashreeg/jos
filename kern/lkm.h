#ifndef JOS_KERN_LKM_H
#define JOS_KERN_LKM_H
#define MOD_NAME_LEN 32


#define NLKM 10 /*support only 10 LKMs*/

#include <inc/elf.h>

struct Module{
	uint64_t *init;
	uint64_t *deinit;
	uint64_t *loadAddress;
	char name[MOD_NAME_LEN];
	char version[MOD_NAME_LEN];
	char author[MOD_NAME_LEN];
	struct Module *link;
	int isLoaded;
	//TODO state
};

void lkm_init(void);
int load_module(char *buffer, char *path);
int unload_module(char *path);
int fix_ksymbols();
int print_symboltable(void);
int fix_symbols();
int print_sht(void);
int create_Module();
int apply_relocation(uint64_t *loadaddress,struct Elf *elf);
int print_modules(int onlyLoaded);
int allocate_memory(int i);
int free_module();
int load_program(int c);
int get_meta(int c);
int list_module();
int lookup_module(char *mname);
int remove_module(int c);
struct Module *modules;

#endif
