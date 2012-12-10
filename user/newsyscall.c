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
sys_screensaver(){
	int row, c, n, temp;
	n = 10;
	temp = n;

	for ( row = 1 ; row <= n ; row++ )
	{
		for ( c = 1 ; c < temp ; c++ )
			cprintf(" ");

		temp--;

		for ( c = 1 ; c <= 2*row - 1 ; c++ )
			cprintf("*");

		cprintf("\n");
	}
	return 0;
}

int
init_module(){
	//insert in ksyms
	cprintf("in newsyscall init module\n");
	put_ksyms("sys_hello_me",(uint64_t)&sys_hello_me);
	put_ksyms("sys_screensaver",(uint64_t)&sys_screensaver);
	return 0;
}

int
clean_module(){
	//delete from ksyms
	cprintf("in newsyscall clean module\n");
	remove_ksyms("sys_hello_me");
	remove_ksyms("sys_screensaver");
	return 0;
}
