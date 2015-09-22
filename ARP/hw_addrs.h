#ifndef __HW_ADDRS_H__
#define __HW_ADDRS_H__

#include "defines.h"

#include <stdio.h>
#include <sys/socket.h>
#include <assert.h>

#define	IP_ALIAS  	 1	/* hwa_addr is an alias */
#define MAX_HWA_NUM   10
#define IPADDR_LEN 32
#define HOSTNAME_LEN 8

typedef struct hwa_info{
	char    if_name[IF_NAME];	/* interface name, null terminated */
	char    if_haddr[IF_HADDR];	/* hardware address */
	int     if_index;		/* interface index */
	short   ip_alias;		/* 1 if hwa_addr is an alias IP address */
	struct  sockaddr  *ip_addr;	/* IP address */
	struct  hwa_info  *hwa_next;	/* next of these structures */
} hwa_info_t;

/* function prototypes */

hwa_info_t *get_eth0_hwa_addr(BOOL print);

void get_host_name(char* hostname, char* ip);

void get_host_addr(char* hostaddr,char* hostname);

#endif
