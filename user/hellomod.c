#include<inc/module.h>
#include<inc/lib.h>

void
init_module(){
	cprintf("hello word. I am a module\n");
}

void
clean_module(){
	cprintf("good bye world. I died\n");
}
