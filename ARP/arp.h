#ifndef __ARP_H__
#define __ARP_H__

#include "defines.h"
#include "hw_addrs.h"
#include <stdint.h>

typedef struct arpmsg {
  uint16_t       id;
  uint16_t	ar_hrd; 	        /* format of hardware address	*/
  uint16_t	ar_pro; 	        /* format of protocol address	*/
  unsigned char	ar_hln;		        /* length of hardware address	*/
  unsigned char	ar_pln;		        /* length of protocol address	*/
  uint16_t	ar_op;		        /* ARP opcode (command)		*/
  unsigned char	ar_sha[ETH_ALEN];	/* sender hardware address	*/
  unsigned char	ar_sip[4];		/* sender IP address		*/
  unsigned char	ar_tha[ETH_ALEN];	/* target hardware address	*/
  unsigned char	ar_tip[4];		/* target IP address		*/
} arp_msg_t;

typedef struct arp_cache_entry {
  struct arp_cache_entry *prev;
  struct arp_cache_entry *next;
  char ip_addr[4];
  unsigned char mac[ETH_ALEN];
  int sll_ifindex;
  int sll_hatype;
  int domain_fd;
} arp_cache_entry_t;

typedef struct arp_cache {
  arp_cache_entry_t * head;
  arp_cache_entry_t * tail;
  int size;
} arp_cache_t;
#endif

