#ifndef __PING_MSG_H__
#define __PING_MSG_H__

#include "defines.h"
#include <stdint.h>

/* ICMP Header + 8 bytes timestamp in Optional Space(icmp->data) */
typedef struct ping_msg {
	uint8_t  type;
	uint8_t  code;
	uint16_t cksum;
	uint16_t id;
	uint16_t seq;
	uint32_t timesec;   /* Time in Seconds */
	uint32_t timeusec;  /* Time in microseconds */
} ping_msg_t;


void parse_icmp_msg(unsigned char *pkt, ping_msg_t *msg);
inline void build_icmp_req(unsigned char *pkt, uint16_t id, uint16_t seq);

#endif
