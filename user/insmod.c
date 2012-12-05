#include<inc/lib.h>
#include<inc/module.h>
#include<inc/elf.h>

#define BUF_SIZE 512
#define MAX_PAGES 10

/*
void insmod(char *path){ // can we send just the file handler
	int f, n;
	char elf_buffer[BUF_SIZE];
	struct Elf *elf;
	int num_pages = 1;

	cprintf("installing module at path %s : \n", path);

	f = open(path, O_RDONLY);
	if(f < 0){
		cprintf("can't open file %s", path);
	}else{

		elf = (struct Elf *)elf_buffer;
		// read the elf and send the binary.
		// ideally should create a module struct and send that
		if ((n = readn(f, elf_buffer, sizeof(elf_buffer)))< 0 || elf->e_magic != ELF_MAGIC)
		{
			cprintf("failed to read elf file: not an elf file\n");
			return;
		}else {
			sys_load_module(elf_buffer, num_pages);
		}

	}
}
*/


// Used for temporary page mappings for mapping KLM binary from user space to kernel space
// (should not conflict with other temporary page mappings)
#define KLMTEMP UTEMP

void insmod(char *path){ // can we send just the file handler
	int f, n, fd;
	struct Elf *elf;
	int r = 0;
	int i = 0;

	//TODO : Check if ELF and relocatable.

	f = open(path, O_RDONLY);
	if(f < 0){
		cprintf("can't open file %s", path);
	}
	else{

		for(i=0; ; i++){

			if(i == MAX_PAGES){
				cprintf("%s: elf file too large to be read\n",path);
				return ;
			}
			cprintf("%s: elf file \n",path);
			if ((r = sys_page_alloc(0, KLMTEMP + i * PGSIZE, PTE_P|PTE_U|PTE_W)) < 0)
				panic("sys_page_alloc: %e", r);
			//cprintf("page allocated for KLM bin: %0x\n",KLMTEMP);
			seek(f, i * PGSIZE);
			if ((n = readn(f, KLMTEMP + i * PGSIZE, PGSIZE )) < PGSIZE ){
				if(n < 0){
					cprintf("failed to read elf file: not an elf file\n");
					return;
				}
				else
					break;
			}

		}
		sys_load_module(KLMTEMP, i+1);
	}

}


void umain(int argc, char **argv){
	binaryname = "insmod";
	int f;
	//TODO: usage and arguments

	//argc must be atleast 1. improve on this check
	if (argc < 2){
		cprintf("Too few arguments\n");
	}else {
		insmod(argv[1]);
	}

}
