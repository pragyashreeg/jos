#include<inc/lib.h>
#include<inc/elf.h>

void lsmod(){ // can we send just the file handler
	sys_list_module();
}


void umain(int argc, char **argv){
	binaryname = "lsmod";
	lsmod();
}
