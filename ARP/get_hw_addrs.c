#include <errno.h>		/* error numbers */
#include <sys/ioctl.h>          /* ioctls */
#include <net/if.h>             /* generic interface structures */
#include <stdbool.h>
#include "unp.h"
#include "hw_addrs.h"

/* Print Pretty, obtained from prt_hwaddrs utility function */
void print_hwa_info(hwa_info_t *hwa, BOOL print)
{
	struct sockaddr	*sa;
	char *ptr;
	int i;
	if (hwa->ip_addr)
	{ 
		if (print)
		{
			printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");

			if ( (sa = hwa->ip_addr) != NULL)
				printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));

			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);

			printf("\n         interface index = %d \n\n", hwa->if_index); 
		}

	}
}

hwa_info_t *
get_eth0_hwa_addr(BOOL print)
{
	struct ifreq	*ifr, *item, ifrcopy;
	struct sockaddr	*sinptr;
	
	char	*buf, lastname[IF_NAME], *cptr ;
	struct ifconf	ifc;
	
	struct hwa_info	*hwa, *hwahead, **hwapnext;
	int	sockfd, len, lastlen, alias, nInterfaces, i;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char*) Malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}

	hwahead = NULL;
	hwapnext = &hwahead;
	lastname[0] = 0;
    
	ifr = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)  {
		item = &ifr[i];
 		alias = 0; 
		hwa = (hwa_info_t *) Calloc(1, sizeof(hwa_info_t));
		memcpy(hwa->if_name, item->ifr_name, IF_NAME);		/* interface name */
		hwa->if_name[IF_NAME-1] = '\0';
				/* start to check if alias address */
		if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon will null */
		if (strncmp(lastname, item->ifr_name, IF_NAME) == 0) {
			alias = IP_ALIAS;
		}
                if (strncmp(hwa->if_name, "eth0", 4) == 0)
                {
                  memcpy(lastname, item->ifr_name, IF_NAME);
                  ifrcopy = *item;
                  *hwapnext = hwa;		/* prev points to this new one */
                  hwapnext = &hwa->hwa_next;	/* pointer to next one goes here */

                  hwa->ip_alias = alias;	/* alias IP address flag: 0 if no; 1 if yes */
                  sinptr = &item->ifr_addr;
                  hwa->ip_addr = (struct sockaddr *) Calloc(1, sizeof(struct sockaddr));
                  memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));	/* IP address */
                  if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0)
                    perror("SIOCGIFHWADDR");	/* get hw address */
                  memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IF_HADDR);
                  if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0)
                    perror("SIOCGIFINDEX");	/* get interface index */
                  memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
                  print_hwa_info(hwa, print);
                }
                else
                  free(hwa);

        }
        free(buf);
        return hwahead;
}

/* host_name buffer should be atleast HOSTNAME_LEN long */
void get_host_name(char* host_name,  char* ip_addr)
{
  struct in_addr ina;
  struct hostent *hptr; 

  Inet_pton(AF_INET, ip_addr, &ina);
  
  if (NULL != (hptr = gethostbyaddr((const char*)&ina, 4, AF_INET))) 
  {
    strncpy(host_name, hptr->h_name, HOSTNAME_LEN);
  }
  else 
    host_name[0] = '\0';
}

/* host_addr buffer should be atleast IPADDR_LEN long */
void get_host_addr(char* host_addr, char* hostname)
{
  struct hostent * hptr; 
  char **pptr; 
  if ((hptr = gethostbyname(hostname)))
  {
    assert(AF_INET == hptr->h_addrtype);
    pptr = hptr -> h_addr_list;
    assert(*pptr);
    Inet_ntop(AF_INET, *pptr, host_addr, IPADDR_LEN);
  }
}

