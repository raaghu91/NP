#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "unp.h"
#include "hw_addrs.h"
#include "defines.h"
#include "hwaddr_def.h"
#include <stdint.h>

typedef struct route_msg {
    	unsigned char mcast_addr[4];
  	uint16_t mcast_port;
	uint8_t num_nodes;
	uint8_t cur_ptr;
	unsigned char ip_list[256*4]; /* 256 entries, as much as can be addressed by num_nodes */
} route_msg_t;

void build_route_msg(unsigned char *pkt_offset, unsigned char* mcast_addr, uint16_t mcast_port, 
                                    uint32_t* ip_list, uint8_t num_nodes, uint8_t cur_ptr);

#endif
