#include "arp.h"
#include "hw_addrs.h"
#include "unp.h"
#include "hwaddr_def.h"
#include <linux/if_arp.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <linux/if_arp.h>
#include "hwaddr_def.h"

hwa_info_t *hwa_head;
static char broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 

/* Sent to the Caller of areq routine */
void build_hwaddr_msg(hwaddr_t *msg, int sll_ifindex, uint16_t sll_hatype, 
			unsigned char sll_halen, unsigned char* sll_addr)
{
    msg->sll_ifindex = sll_ifindex;
    msg->sll_hatype = sll_hatype;
    msg->sll_halen = sll_halen;
    memcpy(msg->sll_addr, sll_addr, ETH_ALEN);
}

/* ARP cache implementation */
arp_cache_t arp_cache;

void build_arp_msg(arp_msg_t *msg, uint16_t ar_op, unsigned char* ar_sha, unsigned char *ar_sip,
                            unsigned char* ar_tha, unsigned char* ar_tip)
{
  assert(msg);
  memset(msg, 0, sizeof(*msg));

  msg->id = htons(ARP_UNIQ_ID);
  msg->ar_hrd = htons(ARPHRD_ETHER);
  msg->ar_pro = htons(PROTO_TYPE_IP);
  msg->ar_hln = ETH_ALEN;
  msg->ar_pln = 4;
  msg->ar_op = htons(ar_op);
  memcpy(msg->ar_sha, ar_sha, ETH_ALEN);
  memcpy(msg->ar_sip, ar_sip, 4);
  if (ar_tha)
    memcpy(msg->ar_tha, ar_tha, ETH_ALEN);
  memcpy(msg->ar_tip, ar_tip, 4);
}

void parse_arp_packet(unsigned char* packet, arp_msg_t* msg) 
{
  memcpy(msg, packet, sizeof(*msg));

  msg->id = ntohs(msg->id);
  msg->ar_hrd = ntohs(msg->ar_hrd);
  msg->ar_pro = ntohs(msg->ar_pro);
  msg->ar_op = ntohs(msg->ar_op);	
}

static void init_arp_cache(arp_cache_t* arp_cache) 
{
  arp_cache->head = NULL;
  arp_cache->tail = NULL;
  arp_cache->size = 0;
}

void add_to_arp_cache(char* ip_addr, unsigned char* mac, int sll_ifindex, int sll_hatype, int domain_fd) 
{
  assert(ip_addr);

  arp_cache_entry_t* entry = (arp_cache_entry_t*)calloc(1, sizeof(arp_cache_entry_t));
  assert(entry);

  memcpy(entry->ip_addr, ip_addr, 4);
  
  /*Incomplete Entry */
  if(mac)
    memcpy(entry->mac, mac, IF_HADDR);
  
  entry->sll_ifindex = sll_ifindex;
  entry->sll_hatype = sll_hatype;
  entry->domain_fd = domain_fd;

  /* Add to Tail */
  entry->prev = arp_cache.tail;
  
  if (arp_cache.tail)
  { 
    arp_cache.tail->next = entry; 
  }
  if (0 == arp_cache.size)
  { 
    arp_cache.head = entry; 
  }
  
  arp_cache.tail = entry; 

  arp_cache.size += 1;
}

arp_cache_entry_t* find_in_arp_cache(unsigned char* ip_addr) 
{
  arp_cache_entry_t* entry = arp_cache.head;
  assert(ip_addr);

  while (entry)
  {
    if (!memcmp(entry->ip_addr, ip_addr, 4))
    {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

void update_arp_cache(unsigned char* ip_addr, unsigned char* mac, int ifindex)
{
  arp_cache_entry_t* entry = NULL;
  assert(mac);
  if ((entry = find_in_arp_cache(ip_addr)))
  {
    printf("Update arp cache:\n");
    memcpy(entry->mac, mac, ETH_ALEN);
    entry->sll_ifindex = ifindex;
  }	
}

void print_arp_cache_entry(arp_cache_entry_t *entry) 
{
  int i = 0;
  static char str[128];
  unsigned char *ptr = entry->mac;
  
  inet_ntop(AF_INET, (struct in_addr *)entry->ip_addr, str, sizeof(str));
  printf("ipaddr: %s \t ", str);
  
  printf("hwaddr: ");
  i = IF_HADDR;
  do {
    printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
  } while (--i > 0);

  printf("\tsll_ifindex: %d \tsll_hatype: %d \tdomain_fd: %d\n", 
      entry->sll_ifindex, entry->sll_hatype, entry->domain_fd);
}

void print_arp_cache() 
{
  arp_cache_entry_t* entry = arp_cache.head;
  printf("\nARP Cache:\n");
  while (entry)
  {
    print_arp_cache_entry(entry);
    entry = entry->next;
  }
  printf("\n");
}

unsigned char* find_mac_by_ip(char* ip_addr) 
{
  hwa_info_t *iter = NULL;
  struct sockaddr_in *sin = NULL;

  for(iter = hwa_head; iter; iter = iter->hwa_next)
  {
    sin = (struct sockaddr_in *)iter->ip_addr;
    if (memcmp(ip_addr, &sin->sin_addr, 4) == 0)
    {
      return (unsigned char *)iter->if_haddr;
    }
  }
  return NULL;
}

/* Print Pretty */
void print_arp_msg_details(uint16_t ar_op, unsigned char* ar_sha, unsigned char *ar_sip,
                            unsigned char* ar_tha, unsigned char* ar_tip)
{
  int i;
  for (i = 0; i < ETH_ALEN; i++) {
    printf("%02x%s", *(ar_sha+i), (i == ETH_ALEN-1) ? " " : ":");
  }
  
  for (i = 0; i < ETH_ALEN; i++) {
    printf("%02x%s", *(ar_tha+i), (i == ETH_ALEN-1) ? " " : ":");
  }
  printf("arp :\n");

  char src_str[128];
  inet_ntop(AF_INET, (struct in_addr *)ar_sip, src_str, sizeof(src_str));
  char dst_str[128];
  inet_ntop(AF_INET, (struct in_addr *)ar_tip, dst_str, sizeof(dst_str));

  if (ARPOP_REQUEST == ar_op)
  {
     printf("arp who-has %s tell %s\n", dst_str, src_str);
  }
  else
  {
     printf("arp reply %s is-at ", src_str);
     for (i = 0; i < ETH_ALEN; i++) {
        printf("%02x%s", *(ar_sha+i), (i == ETH_ALEN-1) ? "\n" : ":");
     }
  }
}

void send_arp_req(int arp_fd, char* ar_tip)
{
  struct sockaddr_ll sock_addr;
  static char packet[ETH_ARP_PKT_SIZE];
  hwa_info_t *iter = NULL;

  sock_addr.sll_pkttype = PACKET_BROADCAST;
  sock_addr.sll_hatype = ARPHRD_ETHER;
  sock_addr.sll_halen = ETH_ALEN;
  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(ETH_UNIQ_ARP_PROTO);

  memcpy(sock_addr.sll_addr, broadcast_mac, 6);

  memcpy(packet, broadcast_mac, 6);
  *(uint16_t*)(packet+ 2*ETH_ALEN) = htons(ETH_UNIQ_ARP_PROTO);

  for(iter = hwa_head; iter; iter = iter->hwa_next)
  {
    /* Update the Egress Interface */
    sock_addr.sll_ifindex = iter->if_index;

    /* Update the Source MAC */
    memcpy(packet+ETH_ALEN, iter->if_haddr, ETH_ALEN);
  
    struct sockaddr_in *sin = (struct sockaddr_in *)iter->ip_addr;

    build_arp_msg((arp_msg_t *)(packet + ETH_HLEN), ARPOP_REQUEST, 
                iter->if_haddr, (char *)&sin->sin_addr, (unsigned char* )NULL, ar_tip);
    Sendto(arp_fd, packet, sizeof(packet), 0, (SA*)&sock_addr, sizeof(sock_addr));
    print_arp_msg_details(ARPOP_REQUEST, iter->if_haddr, (char *)&sin->sin_addr, broadcast_mac, ar_tip);
  } 
}

void send_arp_rsp(int arp_fd, int egress_ifindex, char* ar_sip, unsigned char* ar_tha, char* ar_tip)
{
  struct sockaddr_ll sock_addr;
  static char packet[ETH_ARP_PKT_SIZE];

  memset(packet, 0, sizeof(packet)); 

  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(ETH_UNIQ_ARP_PROTO);
  sock_addr.sll_hatype = ARPHRD_ETHER;
 
  /* Fill the MAC in the Sock Addr */
  memcpy(sock_addr.sll_addr, ar_tha, ETH_ALEN);
  sock_addr.sll_halen = 6;
  
  sock_addr.sll_pkttype = PACKET_OTHERHOST;

  sock_addr.sll_ifindex = egress_ifindex;
  

  /* Fill the Ethernet Header */
  memcpy(packet, ar_tha, ETH_ALEN);

  unsigned char *ar_sha = find_mac_by_ip(ar_sip);
  /* Update the Source MAC */
  memcpy(packet+ETH_ALEN, ar_sha, ETH_ALEN);
  
  *(uint16_t*)(packet + 2*ETH_ALEN) = htons(ETH_UNIQ_ARP_PROTO);
  
  /*Buid the ARP message */
  build_arp_msg((arp_msg_t *)(packet + ETH_HLEN), ARPOP_REPLY, 
      ar_sha, ar_sip, ar_tha, ar_tip);
  Sendto(arp_fd, packet, sizeof(packet), 0, (SA*)&sock_addr, sizeof(sock_addr));
  print_arp_msg_details(ARPOP_REPLY,ar_sha, ar_sip, ar_tha, ar_tip);
}

/* handle ARP responses */
void handle_arp_response(int arp_fd, arp_msg_t *arp_msg, struct sockaddr_ll *sock_addr)
{
  arp_cache_entry_t *entry = NULL;
  entry = find_in_arp_cache(arp_msg->ar_sip);
  if ((entry == find_in_arp_cache(arp_msg->ar_sip))) 
  {
    hwaddr_t hwaddr_msg;
    update_arp_cache(arp_msg->ar_sip, arp_msg->ar_sha, sock_addr->sll_ifindex);
    print_arp_cache();
    
    /* Send to Caller of areq */
    build_hwaddr_msg(&hwaddr_msg, entry->sll_ifindex, entry->sll_hatype, 
        ETH_ALEN, entry->mac);
    Write(entry->domain_fd, &hwaddr_msg, HWADDR_MSG_LEN);
  }
}

/* Handle an ARP request, send resposnse if it is to one of our IPs, update arp entry if needed*/
void handle_arp_request(int arp_fd, arp_msg_t *arp_msg, struct sockaddr_ll *sock_addr)
{
  unsigned char* my_mac = find_mac_by_ip(arp_msg->ar_tip);
  if (my_mac)
  {
    add_to_arp_cache(arp_msg->ar_sip, arp_msg->ar_sha, sock_addr->sll_ifindex, sock_addr->sll_hatype, -1);
    send_arp_rsp(arp_fd, sock_addr->sll_ifindex, arp_msg->ar_tip,
        arp_msg->ar_sha, arp_msg->ar_sip);
  }
  else {
    /* update cache if it exists */
    arp_cache_entry_t* entry;
    if ((entry= find_in_arp_cache(arp_msg->ar_sip)))
      update_arp_cache(arp_msg->ar_sip, arp_msg->ar_sha, sock_addr->sll_ifindex);
      print_arp_cache();
    }
}

int client_fds[FD_SETSIZE];
#define FREE_SLOT -1
static inline void init_clients_fds()
{
  int i;
  for (i=0; i<FD_SETSIZE; i++)
  {
    client_fds[i] = FREE_SLOT;
  }
}

static inline void add_new_client(int client_fd)
{
  int i = 0;
  for (; i < FD_SETSIZE; i++)
  {
    if (client_fds[i] < 0) {
      client_fds[i] = client_fd;
      break;
    }
  }
  if (i == FD_SETSIZE)
  {
    err_quit("add_new_client: Too many clients");
  }
}

int main(int argc, char** argv) 
{
  int listen_fd, new_conn_fd, arp_fd;
  struct sockaddr_un client_addr, serv_addr;
  socklen_t cli_len;
  fd_set read_set, all;
  int n = 0, nread, max_fd = 0, i; 

  hwa_head = get_eth0_hwa_addr(TRUE);
  
  init_arp_cache(&arp_cache);

  /* Create the PF raw socket */
  arp_fd = Socket(AF_PACKET, SOCK_RAW, htons(ETH_UNIQ_ARP_PROTO));

  /* Create Unix Domain Stream Socket */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sun_family = AF_LOCAL;
  strncpy(serv_addr.sun_path, UNIX_DOMAIN_SOCK_STREAM_PATH, sizeof(serv_addr.sun_path));

  /* Setup Listen Queue */
  listen_fd = Socket(AF_LOCAL, SOCK_STREAM, 0);
  
  remove(UNIX_DOMAIN_SOCK_STREAM_PATH);
  
  Bind(listen_fd, (SA*)&serv_addr, sizeof(serv_addr));
  Listen(listen_fd, LISTENQ);
 
  init_clients_fds();

  FD_ZERO(&all);
  FD_SET(listen_fd, &all);
  FD_SET(arp_fd, &all);
  max_fd = listen_fd > arp_fd? listen_fd: arp_fd;

  while (TRUE) 
  {
    read_set = all;		
    
    if ((n = select(max_fd + 1, &read_set, NULL, NULL, NULL)) < 0)
    {
        if (errno == EINTR)
          continue;
        else
          err_quit("Select Unknown Failure \n");
    }

    assert(n != 0);

    /* Handle New Local Connections */
    if (FD_ISSET(listen_fd, &read_set)) 
    {
      cli_len = sizeof(client_addr);
      new_conn_fd = Accept(listen_fd, (SA *)&client_addr, &cli_len);

      FD_SET(new_conn_fd, &all);
      if (new_conn_fd > max_fd) {
        max_fd = new_conn_fd;
      }

      add_new_client(new_conn_fd);

      if (--n <= 0)
      {
        continue;
      }
    }

    if (FD_ISSET(arp_fd, &read_set)) 
    {
      /* Arp Req/Response over the network */
      struct sockaddr_ll sock_addr;
      socklen_t len = sizeof(sock_addr);
      static unsigned char pkt[ETH_ARP_PKT_SIZE];
      arp_msg_t arp_msg;

      Recvfrom(arp_fd, pkt, ETH_ARP_PKT_SIZE, 0, (SA*)&sock_addr, &len);

      parse_arp_packet(pkt + ETH_HLEN, &arp_msg);

      if (arp_msg.id == ARP_UNIQ_ID)  
      {
        if (arp_msg.ar_op == ARPOP_REQUEST)
        {
          print_arp_msg_details(ARPOP_REQUEST, arp_msg.ar_sha, arp_msg.ar_sip, broadcast_mac, arp_msg.ar_tip);
          handle_arp_request(arp_fd, &arp_msg, &sock_addr);
        }
        else if (arp_msg.ar_op == ARPOP_REPLY)
        {
          print_arp_msg_details(ARPOP_REPLY, arp_msg.ar_sha, arp_msg.ar_sip, arp_msg.ar_sha, arp_msg.ar_tip);
          handle_arp_response(arp_fd, &arp_msg, &sock_addr);
        }
      }
      else
      {
        printf("Unexpected ARP msg ID %d received, ignoring\n", arp_msg.id);
      }

      if (--n <= 0)
      {
        continue;
      }
    }

    /* Serve existing client connections */
    for (i = 0; i < FD_SETSIZE; i++)
    {
      /* If no client at this slot, search on */
      if (client_fds[i] < 0)
        continue;

      if (FD_ISSET(client_fds[i], &read_set))
      {
        struct sockaddr_in sock_addr;
        if ((nread = Read(client_fds[i], &sock_addr, sizeof(struct sockaddr_in))) == 0)
        {
          FD_CLR(client_fds[i], &all);

          /* If the Client closed, clean up */
          close(client_fds[i]);
          client_fds[i] = FREE_SLOT;
        }
        else {
          arp_cache_entry_t *entry;
          static char str[128];

          inet_ntop(AF_INET, &sock_addr.sin_addr, str, sizeof(str));

          if (NULL == (entry = find_in_arp_cache(&sock_addr.sin_addr)))
          {
            printf("%s not found in arp cache, sending arp broadcast\n", str);
            add_to_arp_cache(&sock_addr.sin_addr, NULL, -1, 
                                    ARPHRD_ETHER, client_fds[i]);  
            send_arp_req(arp_fd, &sock_addr.sin_addr);
          }
          else {
            hwaddr_t msg;
            printf("Found ARP entry for %s in cache\n", str);
            build_hwaddr_msg(&msg, entry->sll_ifindex, entry->sll_hatype, 
                ETH_ALEN, entry->mac);
            Write(client_fds[i], &msg, sizeof(hwaddr_t));
            FD_CLR(client_fds[i], &all);
            
            /* Clean up as we are done serving*/
            close(client_fds[i]);
            client_fds[i] = FREE_SLOT;
          }
        }
        if (--n <= 0) {

          break;
        }
      }
    } /* End for */
  } /* End while */
  return 0;		
}
