// LAB 6: Your driver code here
#include <kern/e1000.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>
volatile uint32_t * e1000;

int init_tx();
int init_rx();
int e1000_transmit(char *, int);
int e1000_receive();

/* allocate tx ring array and tx pkt buffers*/
struct tx_desc tx_ring_array[E1000_TXRINGLEN]  __attribute__((aligned(16)));
struct tx_pkt tx_pktbuf_array[E1000_TXRINGLEN]; //TODO: I dont like this struct format. Change this to memory block. typedef it

struct rx_desc rx_ring_array[E1000_RXRINGLEN] __attribute__((aligned(16)));
struct rx_pkt rx_pktbuf_array[E1000_RXRINGLEN];

int
e1000_attach(struct pci_func *pcif){
	pci_func_enable(pcif);
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("mmio at %x\n", pcif->reg_base[0]);
	cprintf("device status register : %x\n", e1000[( E1000_STATUS / E1000_SIZE )]);		
	init_tx();
	init_rx();
	
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
	if (data== NULL || len > E1000_TXBUFSIZE){
		return -E_PKT_BAD;
	}
		
	//add it to the end of the tx ring; copy packet to next buf and update TDT
	// read the tail; next descriptor in the tx ring
	uint32_t tail = e1000[E1000_TDT / E1000_SIZE ];
	// uint32_t curr = (tail + 1) % E1000_TXRINGLEN ;

	struct tx_desc *desc = &(tx_ring_array[tail]);
	// check if curr is actually free.
	if (desc->status & E1000_TXD_STAT_DD) {
		memcpy(KADDR(desc->addr), data, len);
		
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

int 
init_rx(){

	int i;

	// program the Receive address registers. Hardcode with MAC address
	e1000[E1000_RAL / E1000_SIZE ] = 0x12005452;
	e1000[E1000_RAH / E1000_SIZE ] = 0x5634;
	e1000[E1000_RAH / E1000_SIZE ] |= 0x1 << 31; //address valid bit

	//program multicast table array.
	e1000[E1000_MTA / E1000_SIZE ] = 0x0;

	//not configuring interrupts
	// 2 paras in the manual

	//initialize arrays with the desc
	memset(rx_ring_array, 0, sizeof(rx_ring_array));
	memset(rx_pktbuf_array, 0, sizeof(rx_pktbuf_array));
	for (i = 0; i < E1000_RXRINGLEN; i++){
		rx_ring_array[i].addr = PADDR( &(rx_pktbuf_array[i]) ); //TODO: does it actually need a address of operator?? I dont think so.
		rx_ring_array[i].status &= ~E1000_RXD_STAT_DD;
	}

	/* Program the Receive Descriptor Base Address */
	e1000[E1000_RDBAL / E1000_SIZE ] = PADDR(&rx_ring_array);
	e1000[E1000_RDBAH / E1000_SIZE ] = 0x0;

	/* Program the Receive Descriptor Length*/
	e1000[E1000_RDLEN / E1000_SIZE ] = E1000_RXRINGLEN * sizeof(struct rx_desc); /* in bytes */

	/* Program Head and Tail registers */
	e1000[E1000_RDH / E1000_SIZE ] = 0x0;
	e1000[E1000_RDT / E1000_SIZE] = 0x0;

	/*Program the Receive Control Registers*/
	e1000[E1000_RCTL / E1000_SIZE ] |= E1000_RCTL_EN;
	e1000[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_LPE; /*disable long packet*/
	e1000[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_LBM_TCVR;
	e1000[E1000_RCTL / E1000_SIZE ] &= ~0x00000300; //TODO
	e1000[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_MO_3;
	e1000[E1000_RCTL / E1000_SIZE ] |= E1000_RCTL_BAM;
	e1000[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_SZ_2048;
	e1000[E1000_RCTL / E1000_SIZE ]	|= E1000_RCTL_SECRC; /*write up says to strip*/


	return 1;
}

/* SUCCESS: returns length of the buffer read
 * ERROR: negative value.*/
int
e1000_receive(char *data, int max_len){

	if (data==NULL) return -E_RX_BAD;

	// read the tail.
	uint32_t tail = e1000[E1000_RDT / E1000_SIZE];
	struct rx_desc *desc = &rx_ring_array[tail];
	if (desc->status & E1000_RXD_STAT_DD) {// This descriptor needs processing
		/*EOP bit should always be set. we dont allow long packets*/
		assert(desc->status & E1000_RXD_STAT_EOP);
		if (desc->length >= max_len){
			panic("in rx buff size is too small");
			return -E_RX_BAD;
		}
		memcpy(data, KADDR(desc->addr), desc->length);

		/*clear the bits*/
		desc->status &= ~E1000_RXD_STAT_DD;
		desc->status &= ~E1000_RXD_STAT_EOP;

		e1000[E1000_RDT / E1000_SIZE] = (tail + 1) % E1000_RXRINGLEN;

		return desc->length;

	}else { //rx queue is empty
		return -E_RX_EMPTY;
	}

}

