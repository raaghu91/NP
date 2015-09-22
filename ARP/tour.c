#include <stdio.h>
#include "unp.h"
#include "ip_hdr.h"
#include "tour_msg.h"
#include "stddef.h"
#include "hw_addrs.h"
#include "icmp_msg.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/ip_icmp.h>

unsigned char* local_hw_addr = NULL;
static char local_hostname[8];
char *local_ip_addr = NULL;
BOOL stop_ping = FALSE;
BOOL is_last_node = FALSE;
int ping_reply_count = 0;
uint16_t pid = 0;
static uint32_t ping_nodes_list[MAX_PING_NODES];
BOOL is_mcast_member = FALSE;
socklen_t sa_len;
int local_hw_ifindex = 0;
int mcast_recv_sockfd,mcast_send_sockfd, pf_sockfd = 0;
struct sockaddr_in send_addr, recv_addr;

int areq(struct sockaddr *ip_addr, socklen_t sockaddrlen, hwaddr_t *hw_addr) 
{
  int domain_fd, nready, n;
  struct sockaddr_un server_addr;
  struct timeval timeout;
  fd_set all;

  char send_msg[sizeof(struct sockaddr_in)];

  /* Fill in the Server Path Name */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_LOCAL;
  strncpy(server_addr.sun_path, UNIX_DOMAIN_SOCK_STREAM_PATH, sizeof(server_addr.sun_path));

  domain_fd = Socket(AF_LOCAL, SOCK_STREAM, 0);

  if (connect(domain_fd, (SA*)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("[Warn]: Failed connecting to ARP Module..\n");
    return 1;
  }

  /* Write to the Socket */ 
  memcpy(send_msg, ip_addr, sizeof(send_msg));
  Write(domain_fd, send_msg, sizeof(send_msg));

  FD_ZERO(&all);
  while (1)
  {
    FD_SET(domain_fd, &all);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if ((nready = select(domain_fd+1, &all, NULL, NULL, &timeout)) < 0)
    {
      if (errno == EINTR)
        continue;
      else
        err_quit("areq: select: Unknown Failure\n");
    }

    if (0 == nready)
    {
      printf("[Info] Timed Out waiting for ARP response..\n");
      return 1;
    }

    /* Read from the Socket */ 
    n = Read(domain_fd, hw_addr, sizeof(*hw_addr));
    if (0 == n)
    {
      printf("[Warn]: ARP Module Dead..\n");
      return 1;
    }
    break;
  }

  close(domain_fd);

  return 0;
}

int gen_sequence_no()
{
  static int seq = 0;
  seq++;
  return seq;
}

void add_ping_node(unsigned char* nodeAddr)
{
  int i = 0; 
  while(i < MAX_PING_NODES)
  {
    if (ping_nodes_list[i] == 0) {
      memcpy((unsigned char*)&ping_nodes_list[i], nodeAddr, IP_ADDR_BYTES);
    }
    i++;
  }
} 

int find_ping_node(unsigned char* nodeAddr) {
  int i;
  while(i < MAX_PING_NODES)
  {
    if (memcmp((unsigned char*)&ping_nodes_list[i], nodeAddr, IP_ADDR_BYTES) == 0) {
      return i;
    }
    i++;
  }
  return NOT_FOUND;
}

void send_ping(unsigned char *dest_ip)
{
  struct sockaddr_ll sock_addr;
  int ret;
  hwaddr_t hw_addr;
  unsigned char pkt[ETH_PING_PKT_LEN];

  memset(&hw_addr, 0, sizeof(hw_addr));
  
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  memcpy(&dest_addr.sin_addr.s_addr, dest_ip, 4);

  char dest_ip_str[IP_ADDR_LEN];
  Inet_ntop(AF_INET, dest_ip, dest_ip_str, sizeof(dest_ip_str));
  
  char dest_host_name[HOST_NAME_LEN];
  get_host_name(dest_host_name, dest_ip_str);

  printf("Contacting ARP module for MAC of %s\n", dest_ip_str);
  /* Send a request to the ARP module */
  if ((ret = areq((SA*)&dest_addr, sizeof(dest_addr), &hw_addr)))
  {	
    printf("ARP request for %s failed\n", dest_ip_str);
    return;
  }	
  printf("Got hwaddr: ");
  unsigned char *ptr = hw_addr.sll_addr;
  int i = IF_HADDR;
  do {
    printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
  } while (--i > 0);
  printf("\tfor %s (%s)\n", dest_host_name, dest_ip_str);

  memset(pkt, 0, sizeof(pkt));

  /* Fill eth_type as ETH_P_IP */
  *(uint16_t*)(pkt + 12) = htons(ETH_P_IP);
  
  /* Fill the dest MAC in the actual packet */
  memcpy(pkt, hw_addr.sll_addr, 6);

  /* Fill our src MAC in the actual packet */
  memcpy(pkt + 6, local_hw_addr, 6);

  /* Build the IP header */
  build_ip_hdr(pkt + ETH_HLEN, local_ip_addr, dest_ip_str, sizeof(ping_msg_t), PING_IP_HDR_ID, 
      DFLT_IP_TTL, IPPROTO_ICMP);

  /*Build the ICMP payload */
  build_icmp_req(pkt + ETH_HLEN + IP_HEADER_LEN, pid, gen_sequence_no());

  /* Fill up the sock_addr_ll structure */ 
  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(ETH_P_IP);
  sock_addr.sll_pkttype = PACKET_OTHERHOST;
  
  /* Fill sockaddr from  what we got from areq */
  sock_addr.sll_hatype = hw_addr.sll_hatype;
  sock_addr.sll_ifindex = hw_addr.sll_ifindex;
  sock_addr.sll_halen = hw_addr.sll_halen;
  
  /* Fill the dest MAC in sockaddr_ll */
  memcpy(sock_addr.sll_addr, hw_addr.sll_addr, ETH_ALEN);

  /* Print Pretty */ 
  printf("PING %s (%s) : %d bytes\n", dest_host_name, dest_ip_str, sizeof(ping_msg_t));
  
  /* Send the packet away.. */
  Sendto(pf_sockfd, pkt, ETH_PING_PKT_LEN, 0, (SA*)&sock_addr, sizeof(sock_addr));
}

double time_taken(long int sec, long int usec) {
	struct timeval out,in;
	in.tv_usec = usec;
	in.tv_sec = sec;

	Gettimeofday (&out, NULL);

	if ( (out.tv_usec -= in.tv_usec) < 0) {       /* out -= in */
                --out.tv_sec;
                out.tv_usec += 1000000;
        }
        out.tv_sec -= in.tv_sec;

	return (out.tv_sec * 1000.0 + out.tv_usec / 1000.0);
}

void sig_alarm_handler(int signo) 
{
  unsigned char dst_addr[4];
  int i = 0;

  if (stop_ping == 1) {
    alarm(0);
    return;
  }

  while(i < MAX_PING_NODES)
  {
    if (ping_nodes_list[i] == 0)
      break;
    
    memcpy(dst_addr, &ping_nodes_list[i], 4);
    send_ping(dst_addr);
    i++;
  }

  alarm(1);
}

void start_tour(int route_sockfd, int num_nodes, char ** nodes) 
{
  int i;
  struct addrinfo *addr_info;
  unsigned char mcast_addr[4];

  /* Get Multicast Address */
  Inet_pton(AF_INET, MCAST_DST_ADDR, mcast_addr);

  num_nodes += 1; /* Include the sender info as well */

  char* next_node = nodes[0];

  uint32_t *ip_list = (uint32_t*)calloc(1, num_nodes * sizeof(uint32_t));
 
  char next_ipaddr[IP_ADDR_LEN], next_hostname[HOST_NAME_LEN];
  get_host_addr(next_ipaddr, next_node);
  get_host_name(next_hostname, next_ipaddr);
  
  /* Store our IP first in the list */ 
  Inet_pton(AF_INET, local_ip_addr, ip_list);

  /* Add everyone else */
  char tmp_addr[IP_ADDR_LEN];
  for (i = 0; i < num_nodes-1; i++)
  {
    get_host_addr(tmp_addr, nodes[i]); 
    Inet_pton(AF_INET, tmp_addr, ip_list+1+i);
  }

  /* Build the Tour Packet */
  int payload_len = offsetof(route_msg_t, ip_list) + num_nodes * sizeof(uint32_t);
  int pkt_len = sizeof(ip_hdr_t) + payload_len;

  unsigned char *pkt = (unsigned char *)calloc(1, pkt_len);
  
  build_ip_hdr(pkt, local_ip_addr, next_ipaddr, payload_len, ROUTE_IP_HDR_ID, DFLT_IP_TTL, IPPROTO_NP4);
  build_route_msg((pkt + sizeof(ip_hdr_t)), mcast_addr, MCAST_DST_PORT, ip_list, num_nodes, 0);

  addr_info = Host_serv(next_ipaddr, NULL, 0, 0);
  
  /* Now, send it forward */
  Sendto(route_sockfd, pkt, pkt_len, 0, addr_info->ai_addr, addr_info->ai_addrlen);
  
  /* Cleanup */
  freeaddrinfo(addr_info);
  
  printf("%s : Send Source Routed msg via %s\n", local_hostname, next_hostname);
  
  free(ip_list);
  free(pkt);
}

void handle_route(int route_sockfd)
{
  ip_hdr_t ip_hdr;
  unsigned char rcv_pkt[sizeof(ip_hdr_t)+sizeof(route_msg_t)];
  
  memset(rcv_pkt, 0, sizeof(rcv_pkt));

  Recvfrom(route_sockfd, rcv_pkt, sizeof(rcv_pkt), 0, NULL, NULL);
  
  parse_ip_hdr(rcv_pkt, &ip_hdr);
  
  if (ROUTE_IP_HDR_ID == ip_hdr.id)
  {
    char src_ipaddr[IP_ADDR_LEN], src_host_name[HOST_NAME_LEN];

    memset(src_ipaddr, 0, sizeof(src_ipaddr));

    Inet_ntop(AF_INET, ip_hdr.src_ip, src_ipaddr, sizeof(src_ipaddr));

    char cur_time[CURR_TIME_LEN];

    time_t ticks;
    ticks = time(NULL);
    
    snprintf(cur_time, sizeof(cur_time), "%.24s", ctime(&ticks));

    get_host_name(src_host_name, src_ipaddr);
    printf("<%s> Received source routing packet from %s (%s)\n", cur_time, src_host_name, src_ipaddr);

    route_msg_t *route_msg = (route_msg_t *)(rcv_pkt+sizeof(ip_hdr_t));

    /* Join the multicast group if not already a member */
    if (!is_mcast_member)
    {
      char mcast_ipaddr[IP_ADDR_LEN];

      Inet_ntop(AF_INET, route_msg->mcast_addr, mcast_ipaddr, sizeof(mcast_ipaddr));

      printf("%s Join multicast group:%s:%d\n", local_hostname, mcast_ipaddr, ntohs(route_msg->mcast_port));
      
      memset(&send_addr, 0, sizeof(send_addr));

      send_addr.sin_family = AF_INET;
      
      /* Fill the Socket Addr with the MCAST details */
      send_addr.sin_port = route_msg->mcast_port;
      Inet_pton(AF_INET, mcast_ipaddr, &send_addr.sin_addr.s_addr);

      memcpy(&recv_addr, &send_addr, sa_len);
     
      /* Bind to the Addr above */
      Bind(mcast_recv_sockfd, (SA*)&recv_addr, sa_len);
      
      is_mcast_member = TRUE;
      
      /* Join the multicast group defined in the the packet */ 
      Mcast_join(mcast_recv_sockfd, (SA*)&send_addr, sa_len, NULL, local_hw_ifindex);
    }

    route_msg->cur_ptr += 1;
    
    if (route_msg->cur_ptr == route_msg->num_nodes - 1) 
    {
      is_last_node = TRUE;
      printf("Node %s. Reached the Last Node of the Tour..\n", local_hostname);
    }
    else
    {
      /* Intermediate Node, continue SSRR */
      unsigned char send_pkt[sizeof(ip_hdr_t)+sizeof(route_msg_t)];

      char next_host_name[HOST_NAME_LEN], next_addr[IP_ADDR_LEN];

      Inet_ntop(AF_INET, route_msg->ip_list + (route_msg->cur_ptr+1)*sizeof(uint32_t), 
                                next_addr, sizeof(next_addr));


      /* Re-build the Tour Packet */
      int num_nodes = route_msg->num_nodes;
      int payload_len = offsetof(route_msg_t, ip_list) + num_nodes * sizeof(uint32_t);
      int pkt_len = sizeof(ip_hdr_t) + payload_len;
      
      memset(send_pkt, 0, pkt_len);

      build_ip_hdr(send_pkt, local_ip_addr, next_addr, payload_len, ROUTE_IP_HDR_ID, ip_hdr.ttl - 1, IPPROTO_NP4);
      
      memcpy(send_pkt + sizeof(ip_hdr_t), rcv_pkt + sizeof(ip_hdr_t), payload_len);

      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = 0;
      Inet_pton(AF_INET, next_addr, &addr.sin_addr);

      Sendto(route_sockfd, send_pkt, pkt_len, 0, (SA*)&addr, sizeof(addr));

      get_host_name(next_host_name, next_addr);
      printf("%s : Send route msg to %s\n", local_hostname, next_host_name);

    }

    if (NOT_FOUND == find_ping_node(ip_hdr.src_ip))
    {
      add_ping_node(ip_hdr.src_ip);
      
      /* Send the First Ping packet */
      sig_alarm_handler(SIGALRM);
    }
  }
  else
  {
    printf("Invalid SSRR packet with ID %d received, ignoring..\n", ip_hdr.id);
  }
}

int handle_ping(int ping_sockfd)
{
  ip_hdr_t ip_hdr;
  static char mcast_msg[100];

  unsigned char recv_pkt[MAXLINE];
  /* Read in the ECHO reply */
  Recvfrom(ping_sockfd, recv_pkt, MAXLINE, 0, NULL, NULL);
  
  parse_ip_hdr(recv_pkt, &ip_hdr);

  ping_msg_t ping_msg;
  parse_icmp_msg(recv_pkt + sizeof(ip_hdr_t), &ping_msg);
  
  switch (ping_msg.type)
  {
    case ICMP_ECHO:
        //printf("Not Expecting ICMP_ECHO, gracefully ignore.. ");
        break;
    case ICMP_ECHOREPLY:  
        if (ping_msg.id != pid)
        {
          //printf("Ping Reply with different ID(%d) than we sent, dropping.. ", ping_msg.id);
          return 0;
        }
        
        char src_ip[IP_ADDR_LEN], tmp_host_name[HOST_NAME_LEN];
        
        /* Calculate the RTT */
        double rtt;
        rtt = time_taken(ping_msg.timesec, ping_msg.timeusec);
        
        /* Get the Src IP from the IP Hdr */ 
        Inet_ntop(AF_INET, ip_hdr.src_ip, src_ip, sizeof(src_ip));

        get_host_name(tmp_host_name, src_ip);

        /* Print Pretty */
        printf("%d bytes from %s (%s): seq=%d, ttl=%d, rtt=%.3f ms\n", 
            sizeof(ping_msg_t), tmp_host_name, src_ip, ping_msg.seq, ip_hdr.ttl, rtt);

        ping_reply_count ++;
        
        if (is_last_node)
        {
          if (ping_reply_count > MAX_PING_COUNT) 
          {
            /* Send the Following message to the mcasst group */
            snprintf(mcast_msg, 100, "<<< This is node %s. Tour has ended. Group members please "
                "identify yourselves. >>>", local_hostname);
            
            /* Print pretty */
            printf("Node %s. Sending: %s\n", local_hostname, mcast_msg);
            
            /* Send it.. */
            Sendto(mcast_send_sockfd, mcast_msg, strlen(mcast_msg)+1, 0, (SA*)&send_addr, sa_len);
	    
            ping_reply_count = 0;
            
            return 1;
          }
        }
        break;
    default: 
        printf("Unhandled ICMP type(%d), dropping.. ", ping_msg.type);
  }
  return 0;
}

void handle_mcast()
{
  char mcast_msg[MAXLINE];
  unsigned char data[100];
  memset(data, 0, sizeof(data));

  Recvfrom(mcast_recv_sockfd, data, sizeof(data), 0, NULL, NULL);

  printf("\n");
  printf("Node %s. Received: %s\n", local_hostname, data);
  
  snprintf(mcast_msg, sizeof(mcast_msg), 
      "<<< Node %s. I am a member of the group. >>>", local_hostname);

  printf("Stopped Pinging, waiting for 5 seconds before termination..\n\n");
  /* Send group advertisement */
  printf("Node %s. Sending: %s\n", local_hostname, mcast_msg);
  Sendto(mcast_send_sockfd, mcast_msg, strlen(mcast_msg)+1, 0, (SA*)&send_addr, sa_len);
}

int main(int argc, char **argv) 
{
  const int on = 1;
  int i=0,route_sockfd, ping_sockfd;
  BOOL source = FALSE;
  hwa_info_t *hwa_info;

  fd_set fdset;
  int max_fd, num_ready;
  struct timeval timeout;
  pid = getpid() & 0xffff;
  
  unsigned char recv_data[100];
  memset(recv_data, 0, sizeof(recv_data));

  if (argc < 2) {
    printf("Tour, running in destination mode\nFor source mode run as:\n <ragkumar_tour [vm1 ... vm10]...>\n");
  }
  else{
    for( i=1; i < argc; i++)
    {

      if((strcmp(argv[i],"vm1") && strcmp(argv[i],"vm2") && strcmp(argv[i],"vm3") && \
            strcmp(argv[i],"vm4") && strcmp(argv[i],"vm5") && strcmp(argv[i],"vm6") && \
            strcmp(argv[i],"vm7") && strcmp(argv[i],"vm8") && strcmp(argv[i],"vm9") && \
            strcmp(argv[i],"vm10")))
      {

        printf("Invalid input\nUsage: <ragkumar_tour [vm1 ... vm10]...>\n");
        return 1;
      }
    }
  }

  if(i == argc) {
    printf("Tour, running in source mode\n");
    source = TRUE;
  }

  // Init self ip & name
  hwa_info = get_eth0_hwa_addr(FALSE);
  if (hwa_info == NULL) {

    err_quit("get_eth0_hwa_addr failed");

  }

  local_hw_addr = hwa_info->if_haddr;
  local_hw_ifindex = hwa_info->if_index;
  local_ip_addr = Sock_ntop_host(hwa_info->ip_addr, sizeof(*(hwa_info->ip_addr)));
  get_host_name(local_hostname, local_ip_addr);
  printf("local node: %s (%s)\n", local_hostname, local_ip_addr);

  Signal(SIGALRM, sig_alarm_handler);
  timeout.tv_usec = 0;
  timeout.tv_sec = 5;

  /* ICMP  socket */
  ping_sockfd = Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  
  /* Raw socket */
  pf_sockfd = Socket(PF_PACKET, SOCK_RAW, ETH_P_IP);

  sa_len = sizeof(send_addr);

  mcast_recv_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
  Setsockopt(mcast_recv_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  memset(&recv_addr, 0, sa_len);

  mcast_send_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
  memset(&send_addr, 0, sa_len);

  /* SSRR Simulation socket  */
  route_sockfd = Socket(AF_INET, SOCK_RAW, IPPROTO_NP4);
  
  /* Set the HDR_INCLUDE option */
  Setsockopt(route_sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));

  if (source == 1) 
  {
    send_addr.sin_family = AF_INET;
   
    /* Fill the Socket Addr with the MCAST details */
    send_addr.sin_port = htons(MCAST_DST_PORT);
    Inet_pton(AF_INET, MCAST_DST_ADDR, &send_addr.sin_addr.s_addr);
    memcpy(&recv_addr, &send_addr, sa_len);
   
    /* Bind to the Addr above */
    Bind(mcast_recv_sockfd, (SA*)&recv_addr, sa_len);
    
    is_mcast_member = TRUE;
    
    /* Join the MCAST group */
    Mcast_join(mcast_recv_sockfd, (SA*)&send_addr, sa_len, NULL, local_hw_ifindex);

    /* Send the First Tour Packet */
    start_tour(route_sockfd, argc-1, argv+1);
  }

  timeout.tv_usec = 0;
  timeout.tv_sec = 5;
  Signal(SIGALRM, sig_alarm_handler);


  FD_ZERO(&fdset);
  while (1) {
    FD_SET(ping_sockfd, &fdset);
    FD_SET(route_sockfd, &fdset);

    max_fd = (route_sockfd > ping_sockfd) ? route_sockfd : ping_sockfd;
    //listen on a mcast recv socket too if part of mcast group
    if (is_mcast_member) {
      FD_SET(mcast_recv_sockfd, &fdset);
      max_fd = (mcast_recv_sockfd > max_fd) ? mcast_recv_sockfd : max_fd;
    }

    if ((num_ready = select(max_fd+1, &fdset, NULL, NULL, NULL)) < 0)
    {
        if (EINTR == errno)
            continue;
        else
          err_quit("main: select: Unknown Error\n");
    }

    if (FD_ISSET(route_sockfd, &fdset)) {
      handle_route(route_sockfd);	
    }

    if (FD_ISSET(ping_sockfd, &fdset)) {
     /* if(handle_ping(ping_sockfd))
	break;	*/	
	handle_ping(ping_sockfd);
    }

    if (FD_ISSET(mcast_recv_sockfd, &fdset)) {
      handle_mcast();
      /* route and ping sockets are no more needed*/
      break;
    }

  }
  /* listen on mcast recv socket again */
  FD_ZERO(&fdset);
  max_fd = mcast_recv_sockfd; 
  while (1) {
    FD_SET(mcast_recv_sockfd, &fdset);
    if ((num_ready = select(max_fd+1, &fdset, NULL, NULL, &timeout)) < 0)
    {
      if (errno == EINTR)
        continue;
      else
        err_quit("mcast_recv:select: Unknown Error\n");
    }
    if (0 == num_ready)
    {
      printf("\nExiting Tour..\n");
      break;
    }
    else
    {
      Recvfrom(mcast_recv_sockfd, recv_data, 100, 0, NULL, NULL);
      printf("Node %s. Received: %s\n", local_hostname, recv_data);
    }
  }

  return 0;
}


