#ifndef JOS_KERN_LKM_H
#define JOS_KERN_LKM_H
/*This is the crux of the LKM*/

struct Module{
	uint64_t *init;
	uint64_t *deinit;
	//TODO state
};

int load_module(char *buffer, int size);
int unload_module(char *buffer);



#endif
