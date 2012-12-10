#include<inc/module.h>
#include<inc/lib.h>

void umain(int argc, char **argv){
	binaryname = "stars";
	char *func = "sys_screensaver";
	sys_call_module((void *)func);

}

