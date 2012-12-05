#include<inc/lib.h>
#include<inc/module.h>
#include<inc/elf.h>

void rmmod(char *buffer){ // can we send just the file handler
	sys_unload_module(buffer);
}


void umain(int argc, char **argv){
	binaryname = "rmmod";
	int f;
	//TODO: usage and arguments

	//argc must be atleast 1. improve on this check
	if (argc < 2){
		cprintf("Too few arguments\n");
	}else {
		rmmod(argv[1]);
	}

}
