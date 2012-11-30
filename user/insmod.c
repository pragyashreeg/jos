#include<inc/lib.h>
#include<inc/module.h>
#include<inc/elf.h>

#define BUF_SIZE 512
void insmod(char *path){ // can we send just the file handler
	int f, n;
	char elf_buffer[BUF_SIZE];
	struct Elf *elf;

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
			sys_load_module(elf_buffer);
		}


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
