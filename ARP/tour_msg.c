#include "tour_msg.h"
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

void build_route_msg(unsigned char *pkt_offset, unsigned char* mcast_addr, uint16_t mcast_port, 
                                    uint32_t* ip_list, uint8_t num_nodes, uint8_t cur_ptr) 
{
  assert(pkt_offset);
  assert(cur_ptr <= num_nodes);

  route_msg_t *msg = (route_msg_t *)pkt_offset;

  memcpy(msg->mcast_addr, mcast_addr, 4);

  msg->mcast_port = htons(mcast_port);

  msg->num_nodes = num_nodes;
  msg->cur_ptr = cur_ptr;

  memcpy(msg->ip_list, (unsigned char*)ip_list, num_nodes * sizeof(uint32_t));
}
