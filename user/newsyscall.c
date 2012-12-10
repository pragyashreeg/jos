#include <inc/module.h>
#include <kern/ksyms.h>


MODULE_VERSION("1.0")
MODULE_AUTHOR("mpaul")

int
sys_hello_me(){
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	cprintf("Hello me , Hello me , Hello me!!\n");
	return 0;
}

int
init_module(){
	//insert in ksyms
	cprintf("in newsyscall init module\n");
	put_ksyms("sys_hello_me",(uint64_t)&sys_hello_me);
	//do a putsyms draw a fancy stars
	return 0;
}

int
clean_module(){
	//delete from ksyms
	cprintf("in newsyscall clean module\n");
	remove_ksyms("sys_hello_me");
	//do a removesyms draw a fancy stars
	return 0;
}
