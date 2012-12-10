#ifndef JOS_INC_MODULE_H
#define JOS_INC_MODULE_H

/*int sys_load_module(char *, char *path);
int sys_unload_module(char *buffer);
int sys_ls_module();*/

int	cprintf(const char *fmt, ...);
int sys_hello_me(); // this shud be exposed by the newsyscall apis.

#define MODULE_VERSION(v) char *version = v;
#define MODULE_AUTHOR(n) char *author = n;

#endif /*JOS_INC_MODULE_H*/
