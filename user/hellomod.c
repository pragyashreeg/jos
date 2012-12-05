#include<inc/kernel.h>

int
init_module(){
	cprintf("hello word. I am a module\n");
	return 0;
}

int
clean_module(){
	cprintf("bye world. I am a dead module\n");
	return 0;
}

