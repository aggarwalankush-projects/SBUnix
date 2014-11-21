#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

#define E1000_VID 0x8086
#define E1000_DID 0x100e

#define BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64
#define PACKET_SIZE 1518  //maximum size of an Ethernet packet is 1518 bytes.
#define RX_PACKET_SIZE 2048


#define E1000_SIZE sizeof(*e1000_mmio_addr)

//Registers

#define E1000_STATUS   0x00008 

//Transmit Descriptor registers
#define E1000_TCTL 0x00400
#define E1000_TIPG 0x00410
#define E1000_TDBAL 0x03800
#define E1000_TDBAH 0x03804
#define E1000_TDLEN 0x03808
#define E1000_TDH 0x03810
#define E1000_TDT 0x03818

/* Transmit Control */
#define E1000_TCTL_EN 0x00000002    /* enable tx */
#define E1000_TCTL_PSP 0x00000008  /* pad short packets */
#define E1000_TCTL_CT 0x00000ff0  /* collision threshold */
#define E1000_TCTL_COLD 0x1000000 /* collision distance */

/* Transmit Descriptor bit definitions */
#define E1000_TX_CMD_RS 0x00000008 /* Report Status */
#define E1000_TX_CMD_EOP 0x00000001  /* End of Packet */
#define E1000_TX_STAT_DD 0x00000001  /* Descriptor Done */

//Receive Descriptor registers
#define E1000_RAL      0x05400  /* Receive Address Low */
#define E1000_RAH 	   0x05404  /* Receive Address high */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */

//Receive Control
#define E1000_RCTL     0x00100  /* RX Control - RW */
//#define E1000_RCTL_RST            0x00000001    /* Software reset */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */
#define E1000_RCTL_LBM_TCVR       0x000000C0    /* tcvr loopback mode */
#define E1000_RCTL_MO_3           0x00003000    /* multicast offset 15:4 */
#define E1000_RCTL_MDR            0x00004000    /* multicast desc ring 0 */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */

//RX bit definitions
#define E1000_RX_STAT_DD       0x01    /* Descriptor Done */

int e1000_attach(struct pci_func *);
int e1000_initialize();
int e1000_transmit(char *data, int len);
int e1000_receive(char *data);

struct tx_desc
{
	uint64_t buffer_addr;
	uint16_t length;   /* Data buffer length */
	uint8_t cso; /* Checksum offset */
	uint8_t cmd;  /* Descriptor control */
	uint8_t css;  /* Checksum start */
	uint8_t status;  /* Descriptor status */
	uint16_t special;
};

struct rx_desc
{
	uint64_t buffer_addr;
	uint16_t length;
	uint16_t checksum;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
};

#endif	// JOS_KERN_E1000_H
