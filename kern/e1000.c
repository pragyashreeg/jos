// LAB 6: Your driver code here
#include <kern/e1000.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>
volatile uint32_t * e1000;

int init_tx();
int transmit(char *, int);

/* allocate tx ring array and tx pkt buffers*/
struct tx_desc tx_ring_array[E1000_TXRINGLEN]  __attribute__((aligned(16)));
struct tx_pkt tx_pktbuf_array[E1000_TXRINGLEN];

int
e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("mmio at %x\n", pcif->reg_base[0]);
	cprintf("device status register : %x\n", e1000[( E1000_STATUS / E1000_SIZE )]);		
	init_tx();
	
	//Test Transmit
	#if 0
	char *str = "hello world";
	int ret;
	ret = e1000_transmit(str, strlen(str));	
	cprintf("return val : %d\n", ret);
	#endif
	return 1;
}

int 
init_tx(){
	
	int i;
	//initialize the tx arrays
	memset(tx_ring_array, 0, sizeof(tx_ring_array));
	memset(tx_pktbuf_array, 0, sizeof(tx_pktbuf_array));
	for(i = 0; i < E1000_TXRINGLEN; i++){
		tx_ring_array[i].addr = PADDR(& (tx_pktbuf_array[i]) );
		tx_ring_array[i].status |= E1000_TXD_STAT_DD; /*tx checks on this bit to conclude if the desc is empty or not*/    
	}	

	//program the hardware
	
	/*TX Descriptor Base Address registers */
	e1000[E1000_TDBAL / E1000_SIZE] = PADDR(tx_ring_array) ;
	e1000[E1000_TDBAH / E1000_SIZE] = 0x0;

	/* TX Descriptor Length */
	e1000[E1000_TDLEN / E1000_SIZE] = E1000_TXRINGLEN * sizeof (struct tx_desc); /*in bytes*/
	
	/*TX Descriptor Head and Tail pointer registers*/	
	e1000[E1000_TDH / E1000_SIZE] = 0x0;
	e1000[E1000_TDT / E1000_SIZE] = 0x0;

	/*TX Control Registers*/
	e1000[E1000_TCTL / E1000_SIZE ] |= E1000_TCTL_EN;
	e1000[E1000_TCTL / E1000_SIZE ] |= E1000_TCTL_PSP;
	e1000[E1000_TCTL / E1000_SIZE ] &= ~E1000_TCTL_CT; // clear the bits
	e1000[E1000_TCTL / E1000_SIZE ] |= (0x10) << 4; //set it to 10h
	e1000[E1000_TCTL / E1000_SIZE ]	&= ~E1000_TCTL_COLD;
	e1000[E1000_TCTL / E1000_SIZE ] |= (0x40) << 12;//set to 40h. full duplex
	
	/*TX IPG Registers*/
	e1000[E1000_TIPG / E1000_SIZE ] |= (0x10); // IPGT 0:9
	e1000[E1000_TIPG / E1000_SIZE ] |= (0x6) << 20; //IPGR2 20:29
	e1000[E1000_TIPG / E1000_SIZE ]	|= (0x4) << 10; //IPGR1 10:19
	return 1;
}

/*returns 0 on success -ve values on error*/
int
e1000_transmit(char *data, int len){
	
	//if len tooo big return error	
	if (len > E1000_TXBUFSIZE){
		return -E_PKT_BAD;
	}
		
	//add it to the end of the tx ring; copy packet to next buf and update TDT
	// read the tail; next descriptor in the tx ring
	uint32_t tail = e1000[E1000_TDT / E1000_SIZE ];
	uint32_t curr = (tail + 1) % E1000_TXRINGLEN ;

	struct tx_desc *desc = &(tx_ring_array[tail]);
	// check if curr is actually free.
	if (desc->status & E1000_TXD_STAT_DD) {
		memcpy(KADDR(desc->addr), data, len);
		
		cprintf("%x\n", PADDR(desc));
		desc->length = len;
		desc->cmd |= 0x8;//E1000_TXD_CMD_RS;
		desc->cmd |= 0x1;//E100_TXD_CMD_EOP; 
		desc->status &= ~E1000_TXD_STAT_DD; //clear the DD bit
		//TODO 
		e1000[E1000_TDT / E1000_SIZE] = (tail + 1) % E1000_TXRINGLEN;
			 
	}else { // handle full queue
		-E_TX_FAIL;
	}

	return 0;
}

#if 0
int 
init_rx(){


	return 1;
}

#endif 

