#ifndef JOS_KERN_LKM_H
#define JOS_KERN_LKM_H
/*This is the crux of the LKM*/

int load_module(char *buffer);
int unload_module(char *buffer);

#endif
