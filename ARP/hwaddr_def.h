#ifndef __HWADDR_DEF_H__
#define __HWADDR_DEF_H__

#include <stdint.h>
#include "defines.h"

typedef struct hwaddr {
	int sll_ifindex;		/* Interface number */
	uint16_t  sll_hatype;		/* Hardware type */
	unsigned char sll_halen;	/* Length of address */
	unsigned char sll_addr[8];	/* Physical layer address */
} hwaddr_t;

#endif
