#include "ns.h"
#include <kern/e1000.h>
extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int max_len = E1000_RXBUFSIZE, r, len;
	char data[max_len];
	int perm = PTE_U | PTE_P | PTE_W;

	while (1){
		while( (len = sys_try_rcv_packet(data, max_len)) < 0){
			sys_yield(); // the queue may be empty. avoid spin forever.
		}
		//allocate a diff phys page to the nsipcbuf: HINT
		r = sys_page_alloc(0, &nsipcbuf, perm);
		assert(r == 0);

		//now send the packet to ns
		memcpy(&nsipcbuf.pkt.jp_data, &data, len);
		nsipcbuf.pkt.jp_len = len;
		while ( (r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, perm)) < 0);
	}
}
