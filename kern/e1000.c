#include <kern/e1000.h>
#include <inc/stdio.h>
#include <inc/memlayout.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

// LAB 6: Your driver code here
volatile uint32_t *e1000_mmio_addr;
struct tx_desc tx_desc_list[BUFFER_SIZE];   // Transmit Descriptor list.
struct rx_desc rx_desc_list[RX_BUFFER_SIZE];
char tx_packet_buf[BUFFER_SIZE][PACKET_SIZE];
char rx_packet_buf[BUFFER_SIZE][RX_PACKET_SIZE];

int e1000_attach(struct pci_func *pcif){
	uint32_t reg_status;
	pci_func_enable(pcif); //Exercise 3. Only enable by calling pci_func_enable
	e1000_mmio_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	//reg_status = *(e1000_mmio_addr + 2);
	reg_status = e1000_mmio_addr[( E1000_STATUS / E1000_SIZE )];
	cprintf("Device Status Register : %x\n", reg_status);
	assert(reg_status == 0x80080783);
	e1000_initialize();
	int debug = 0; // Set to test Exercise 6. Unset after exercise 8.
	if(debug == 1)
	{
		char tx_packet_buf[10][8] = {"Packet0","Packet1",
					   "Packet2", 
					   "Packet3", 
					   "Packet4", 
					    "Packet5",
					    "Packet6", 
					    "Packet7",
					    "Packet8",
					    "Packet9"};
		int i;
		for(i = 0; i < 10; i++)
		{
			while(1)
			{
				if(!(e1000_transmit(tx_packet_buf[i], strlen(tx_packet_buf[i]))))
					break;
			}
		}
	}	
	return 0;
}	

int e1000_initialize(){

	int i;
	/*Initalize TX*/
	memset(tx_desc_list, 0, sizeof(tx_desc_list));
	memset(tx_packet_buf, 0, sizeof(tx_packet_buf));
	for(i = 0; i < BUFFER_SIZE; i++){
		tx_desc_list[i].status |= E1000_TX_STAT_DD; /*Check if the desc is empty or not*/ 
		tx_desc_list[i].buffer_addr = PADDR(& (tx_packet_buf[i]) );	   
	}

	//Transmit Descriptor BAR's
	e1000_mmio_addr[E1000_TDBAL / E1000_SIZE] = PADDR(tx_desc_list) ;
	e1000_mmio_addr[E1000_TDBAH / E1000_SIZE] = 0x0;
	//Tranmsit Descriptor Length in bytes
	e1000_mmio_addr[E1000_TDLEN / E1000_SIZE] = BUFFER_SIZE * sizeof (struct tx_desc);
	//TX register Head and Tail pointers
	e1000_mmio_addr[E1000_TDH / E1000_SIZE] = 0x0;
	e1000_mmio_addr[E1000_TDT / E1000_SIZE] = 0x0;
	//Transmit Control Registers
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ] |= E1000_TCTL_EN;
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ] |= E1000_TCTL_PSP;
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ] &= ~E1000_TCTL_CT; // Clear Bits
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ] |= (0x10) << 4; //Set to 10h
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ]	&= ~E1000_TCTL_COLD;
	e1000_mmio_addr[E1000_TCTL / E1000_SIZE ] |= (0x40) << 12;//Full duplex = 40h
	//Transmit IGP Registers
	/*TX IPG Registers*/
	e1000_mmio_addr[E1000_TIPG / E1000_SIZE ] |= (0x10); // IPGT 0:9
	e1000_mmio_addr[E1000_TIPG / E1000_SIZE ] |= (0x6) << 20; //IPGR2 20:29
	e1000_mmio_addr[E1000_TIPG / E1000_SIZE ]	|= (0x4) << 10; //IPGR1 10:19

	/*****
	 Initalize RX

	 *******/
	// Hardcode MAC addr.
	e1000_mmio_addr[E1000_RAL / E1000_SIZE ] = 0x12005452;
	e1000_mmio_addr[E1000_RAH / E1000_SIZE ] = 0x5634;
	e1000_mmio_addr[E1000_RAH / E1000_SIZE ] |= 0x1 << 31; //address valid bit
	//Multicast table array.
	e1000_mmio_addr[E1000_MTA / E1000_SIZE ] = 0x0;
	//initialize arrays with the desc
	memset(rx_desc_list, 0, sizeof(rx_desc_list));
	memset(rx_packet_buf, 0, sizeof(rx_packet_buf));
	for (i = 0; i < RX_BUFFER_SIZE; i++){
		rx_desc_list[i].buffer_addr = PADDR( &(rx_packet_buf[i]));
		rx_desc_list[i].status &= ~E1000_RX_STAT_DD;
	}
	// RX Desc BA
	e1000_mmio_addr[E1000_RDBAL / E1000_SIZE ] = PADDR(&rx_desc_list);
	e1000_mmio_addr[E1000_RDBAH / E1000_SIZE ] = 0x0;
	// Receive Descriptor Length
	e1000_mmio_addr[E1000_RDLEN / E1000_SIZE ] = RX_BUFFER_SIZE * sizeof(struct rx_desc);
	//Head and Tail registers
	e1000_mmio_addr[E1000_RDH / E1000_SIZE ] = 0x0;
	e1000_mmio_addr[E1000_RDT / E1000_SIZE] = 0x0;
	//RX Control Registers
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] |= E1000_RCTL_EN;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_LPE;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_LBM_TCVR;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] &= ~0x00000300; 
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_MO_3;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] |= E1000_RCTL_BAM;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ] &= ~E1000_RCTL_SZ_2048;
	e1000_mmio_addr[E1000_RCTL / E1000_SIZE ]	|= E1000_RCTL_SECRC;
	return 1;
}

//Function to transmit packet
int e1000_transmit(char *data, int len){
	
	uint32_t tail, next_tail;
	tail = e1000_mmio_addr[E1000_TDT/E1000_SIZE];
	next_tail = (tail + 1)%BUFFER_SIZE;
	int status = tx_desc_list[tail].status & E1000_TX_STAT_DD;
	if(!status)
		//cprintf("Ring Fail");
		return -E_TX_QUEUE_FULL;
	memmove(tx_packet_buf[tail], data, len);
	tx_desc_list[tail].length = len;
	tx_desc_list[tail].cmd |= E1000_TX_CMD_RS;
	tx_desc_list[tail].cmd |= E1000_TX_CMD_EOP;
	e1000_mmio_addr[E1000_TDT/E1000_SIZE] = next_tail;

	return 0;
}

int e1000_receive(char *data)
{
	uint32_t rdt, next_rdt;
	rdt = e1000_mmio_addr[E1000_RDT/E1000_SIZE];
	//next_rdt = (rdt+1)%RX_BUFFER_SIZE;
	int status = rx_desc_list[rdt].status & E1000_RX_STAT_DD;
	if(status != 0x1)
		//cprintf("No packet");
		return -E_NO_PCKT;
	int len = rx_desc_list[rdt].length;
	int i;
	/*for(i=0;i<len;i++){
		pkt_buf[i];
		rx_packet_buf[rdt][i];
		cprintf("the I %d\n",i);
	}*/
	//cprintf("\nRDT = %d\n", rdt);
	memmove(data,rx_packet_buf[rdt],len);
	rx_desc_list[rdt].status = 0x0;
	 e1000_mmio_addr[E1000_RDT/E1000_SIZE] = (rdt+1)%RX_BUFFER_SIZE;
	return len; 
}