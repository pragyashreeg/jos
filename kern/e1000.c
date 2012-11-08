#include <kern/e1000.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <kern/pmap.h>
// LAB 6: Your driver code here

volatile uint32_t * e1000;

int
e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("device status register : %x\n", e1000[2]);	
	return 1;
}
