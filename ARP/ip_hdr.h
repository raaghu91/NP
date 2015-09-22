#ifndef __IP_HDR_H__
#define __IP_HDR_H__

#include "defines.h"
#include <stdint.h>
#include <assert.h>

/* Vanilla IP Header */
typedef struct ip_hdr {
	uint8_t version_len;
	uint8_t dscp_ecn;
	uint16_t len;	
	uint16_t id;
	uint16_t df_mf;
	uint8_t ttl;
	uint8_t proto;
	uint16_t cksum;
	unsigned char src_ip[4];
	unsigned char dst_ip[4];
} ip_hdr_t;

extern void build_ip_hdr(unsigned char* hdr, char* src_ip, char* dest_ip, uint16_t length, 
                    uint16_t id, unsigned char ttl, unsigned char proto);

extern void parse_ip_hdr(unsigned char* pkt, ip_hdr_t* ip_hdr);
#endif /* __IP_HDR_H__ */
