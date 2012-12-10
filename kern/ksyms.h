#ifndef JOS_INC_KSYMS
#define JOS_INC_KSYMS

#define MAX_SYMNAME_LEN 30
#define MAX_SYM_NUM 30

#include<inc/types.h>

/*
struct Ksymbol{
	uint32_t name; // index into the string table
	unsigned char info; //
	unsigned char other;
	uint16_t shndx; //
	uint64_t value; // the value of the symbol
	uint64_t size;
};
*/
struct Ksymbol{
	char name[MAX_SYMNAME_LEN];
	uint64_t value;
	int is_valid;
};

int print_ksyms(void);
int load_ksyms(void);
int print_ksyms(void);
int remove_ksyms(char *symbol_name);
int put_ksyms(char *name, uint64_t value);
int get_ksyms(char *symbol_name, struct Ksymbol *symbol_store);
int print_symList(void);

#endif
