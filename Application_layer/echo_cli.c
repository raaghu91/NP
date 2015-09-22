#include "unp.h"

void str_cli(FILE *fp, int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	int maxfdp;
	fd_set rset;

	FD_ZERO(&rset);
	for(;;){
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp = max(fileno(fp), sockfd) + 1;
		Select(maxfdp, &rset, NULL, NULL, NULL);
	
		if(FD_ISSET(sockfd, &rset)) {
			if(Readline(sockfd, recvline, MAXLINE) == 0)
				err_quit("str_cli: server terminated prematurely");
			Fputs(recvline, stdout);
		}
		
		if(FD_ISSET(fileno(fp), &rset)){
			
			if(Fgets(sendline,MAXLINE,fp) == NULL)
				return;
			Writen(sockfd, sendline, strlen(sendline));
		}
	}
}

int main(int argc, char ** argv)
{
        int sockfd,write_pipe;
	char* buf[50];
        struct sockaddr_in servaddr;
	
	
        if(argc < 2)
                err_quit("usage: echo_cli <IPaddress>");

	write_pipe=atoi(argv[2]);	
	dup2(write_pipe,2);
	
        sockfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(1500);
        Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

        Connect(sockfd, (SA *) & servaddr, sizeof(servaddr));

	str_cli(stdin,sockfd);
        exit(0);
}

