#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include "icmp_msg.h"
#include "unp.h"
#include "stddef.h"
#include <assert.h>

void parse_icmp_msg(unsigned char* pkt, ping_msg_t *msg)
{
  assert(msg);
  assert(pkt);

  ping_msg_t *tmp_msg = (ping_msg_t *)pkt;
 
  msg->type = tmp_msg->type;
  msg->code = tmp_msg->code;
  msg->cksum = ntohs(tmp_msg->cksum);
  msg->id = ntohs(tmp_msg->id);
  msg->seq = ntohs(tmp_msg->seq);
  msg->timesec = ntohl(tmp_msg->timesec);
  msg->timeusec = ntohl(tmp_msg->timeusec);
}

static void build_icmp_msg(unsigned char* pkt, unsigned char type, unsigned char code, 
                 uint16_t id, uint16_t seq, uint32_t timesec, uint32_t timeusec) 
{
  struct timeval tval;

  ping_msg_t* msg = (ping_msg_t *) pkt;
  
  assert(msg);

  msg->type = type;
  msg->code = code;
  msg->cksum = 0;
  msg->id = htons(id);
  msg->seq= htons(seq);
  
  Gettimeofday (&tval, NULL);

  if (timesec == 0)
    msg->timesec = htonl(tval.tv_sec);
  else {
    msg->timesec = htonl(timesec);
  }

  if (timeusec == 0)
    msg->timeusec = htonl(tval.tv_usec);
  else
    msg->timeusec = htonl(timeusec);

  /* ICMP Checksum */
  *(uint16_t*)(pkt+offsetof(ping_msg_t, cksum)) = in_cksum((uint16_t*)pkt, PING_MSG_LEN);
}

inline void build_icmp_req(unsigned char *pkt, uint16_t id, uint16_t seq)
{
  build_icmp_msg(pkt, ICMP_MSG_ECHO_REQ_TYPE, ICMP_MSG_ECHO_REQ_CODE, id, seq, 0, 0);
}
