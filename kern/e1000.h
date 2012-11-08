#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VID 0x8086
#define E1000_DID 0x100E

#define E1000_STATUS   0x00008  /* Device Status - RO *///TODO:

int e1000_attach(struct pci_func *pcif);

#endif	// JOS_KERN_E1000_H



