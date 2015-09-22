#include "unp.h"

void date_cli(int sockfd)
{
	int n;
	char recvline[MAXLINE];
	while((n = read(sockfd, recvline, MAXLINE)) > 0){
		recvline[n] = 0;
		if(fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if (n < 0)
		err_sys("read error");
	
	
}

int main(int argc, char ** argv)
{
        int sockfd;
        struct sockaddr_in servaddr;

        if(argc < 2)
                err_quit("usage: time_cli <IPaddress>");
	
	dup2(atoi(argv[2]),2);

        sockfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(1600);
        Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

        Connect(sockfd, (SA *) & servaddr, sizeof(servaddr));

	date_cli(sockfd);
        exit(0);
}

