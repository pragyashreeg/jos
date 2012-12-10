#ifndef JOS_TRY_H
#define JOS_TRY_H

/*
int	cprintf(const char *fmt, ...); /
int hello_me();

int put_ksyms(const char *name,unsigned long long value);
int remove_ksyms(const char *name);

#define MODULE_VERSION(v) char *version = v;
#define MODULE_NAME(n) char *author = n;
*/
int put_ksyms(const char *name,unsigned long long value);
int remove_ksyms(const char *name);

#endif

