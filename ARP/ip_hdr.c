#include "ip_hdr.h"
#include "unp.h"
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "stddef.h"

/* Routine to get the contents of the IP header in Host Order */
void parse_ip_hdr(unsigned char* pkt, ip_hdr_t* ip_hdr)
{
  assert(ip_hdr);
  assert(pkt);

  memcpy(ip_hdr, pkt, sizeof(ip_hdr_t));

  ip_hdr->len = ntohs(ip_hdr->len);
  ip_hdr->id  = ntohs(ip_hdr->id);
}

void build_ip_hdr(unsigned char *header, char* src_ip, char* dst_ip,
                uint16_t len, uint16_t id, uint8_t ttl, uint8_t proto)
{
  ip_hdr_t *ip_hdr = (ip_hdr_t *)header;

  ip_hdr->version_len = (0x4<<4) | 0x5;
  ip_hdr->dscp_ecn = 0;
  ip_hdr->len = htons(len + sizeof(ip_hdr_t));
  ip_hdr->id = htons(id);
  ip_hdr->df_mf = 0;
  ip_hdr->ttl = ttl;
  ip_hdr->proto = proto;
  ip_hdr->cksum = 0;

  Inet_pton(AF_INET, src_ip, ip_hdr->src_ip);

  Inet_pton(AF_INET, dst_ip, ip_hdr->dst_ip);

  /* IP Checksum */
  *(uint16_t*)(header+offsetof(ip_hdr_t, cksum)) = in_cksum((uint16_t*)ip_hdr, sizeof(ip_hdr_t));
}
