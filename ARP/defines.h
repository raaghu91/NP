#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <linux/if_ether.h>

#define TRUE 1
#define FALSE 0
typedef short int BOOL; 

#define UNIX_DOMAIN_SOCK_STREAM_PATH "/tmp/scmrag.49777"

#define HOST_NAME_LEN   32

#define IF_NAME     16  /* same as IFNAMSIZ    in <net/if.h> */
#define IF_HADDR     6  /* same as IFHWADDRLEN in <net/if.h> */
#define IP_ADDR_BYTES 	 4
#define PROTO_TYPE_IP 0x0800

#define IPPROTO_NP4 	35  /* Free, not in IPPROTO_XXX in <netinet/in.h> */

#define ETH_UNIQ_ARP_PROTO  9777
#define ARP_UNIQ_ID         9777
#define ETH_ARP_PKT_SIZE     44  /* Ethernet header + ARP ID + ARP Req/Rsp */

#define MCAST_DST_ADDR "239.255.2.5"
#define MCAST_DST_PORT 9777

#define IP_ADDR_LEN 16
#define ROUTE_IP_HDR_ID 9777
#define PING_IP_HDR_ID 9168

#define HWADDR_MSG_LEN 15

#define IP_HEADER_LEN   20
#define DFLT_IP_TTL     64

#define ROUTE_MSG_HLEN 10

#define ICMP_MSG_ECHO_REQ_TYPE 8
#define ICMP_MSG_ECHO_RSP_TYPE 0

#define ICMP_MSG_ECHO_REQ_CODE 0
#define ICMP_MSG_ECHO_RSP_CODE 0


#define MAX_PING_COUNT 5
#define NOT_FOUND -1
#define PING_MSG_LEN 16
#define MAX_PING_NODES 10
#define ETH_PING_PKT_LEN 50 /* eth_hdr = 14 + ip hdr_t =  20 + ping_msg_t = 16 */


#define CURR_TIME_LEN 48



#endif

