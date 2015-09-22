#include "unp.h"
#include <time.h>
//#include "unpthread.h"



static void *str1_echo(void *sockfd)
{
	int fd;
        ssize_t n;
        char buf[MAXLINE];
	fd=(int)sockfd;
	Pthread_detach(pthread_self());
        again:
        while((n = read(fd, buf, MAXLINE)) > 0)
                Writen(fd,buf,n);
        if(n<0 && errno == EINTR)
                goto again;
        else if (n<0)
                err_sys("str1_echo: read error");
	printf("echo client terminated:socket read returned with value 0\n");
	Close(fd);
	return (NULL);
}

static void * time_echo(void *sockfd)
{	
	int fd,maxfdp;
	fd_set rset,wset;
	Pthread_detach(pthread_self());
	char buf[MAXLINE];
	time_t ticks;
	struct timeval tv;

	fd=(int)sockfd;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	for( ; ; ){

		FD_SET(fd, &rset);
		maxfdp = fd + 1;
		tv.tv_sec=5;
		tv.tv_usec=0;
		ticks = time(NULL);
		snprintf(buf, sizeof(buf), "%.24s\r\n",ctime(&ticks));
		Write(fd, buf, strlen(buf));
		Select(maxfdp, &rset, NULL, NULL, &tv);

		if(FD_ISSET(fd, &rset)){
			printf("time client termination: EPIPE error detected\n");
			FD_CLR(fd,&rset);		
			Close(fd);
			return NULL;
		
		}

	}
	Close(fd);
	return (NULL);
}

int main(int argc, char **argv)
{
	int listenfd_ec, connfd_ec, listenfd_ti, connfd_ti, maxfdp, optval=1, fileflags,fileflags2;
	pthread_t tid;
	fd_set rset;
	struct sockaddr_in cliaddr_ec,servaddr_ec, cliaddr_ti, servaddr_ti;
	socklen_t clilen_ec, clilen_ti;

	if(argc > 1)
	{
		printf("Usage: ./server \n");
		exit(1);
	}

	listenfd_ec = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr_ec, sizeof(servaddr_ec));
	servaddr_ec.sin_family = AF_INET;
	servaddr_ec.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr_ec.sin_port = htons(1500);
	setsockopt(listenfd_ec,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));	
	Bind(listenfd_ec,(SA *) &servaddr_ec, sizeof(servaddr_ec));

	if (fileflags = fcntl(listenfd_ec, F_GETFL, 0) == -1)  {
		   perror("fcntl F_GETFL");
		   exit(1);
	}

	if (fcntl(listenfd_ec, F_SETFL, fileflags | O_NONBLOCK) == -1)  {
		   perror("fcntl F_SETFL, O_NONBLOCK");
		   exit(1);
	}

	Listen(listenfd_ec, LISTENQ);

	listenfd_ti = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr_ti, sizeof(servaddr_ti));
	servaddr_ti.sin_family = AF_INET;
	servaddr_ti.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr_ti.sin_port = htons(1600);
	setsockopt(listenfd_ti,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));	
	Bind(listenfd_ti,(SA *) &servaddr_ti, sizeof(servaddr_ti));

	if (fileflags2 = fcntl(listenfd_ti, F_GETFL, 0) == -1)  {
		   perror("fcntl F_GETFL");
		   exit(1);
	}

	if (fcntl(listenfd_ti, F_SETFL, fileflags2 | O_NONBLOCK) == -1)  {
		   perror("fcntl F_SETFL, O_NONBLOCK");
		   exit(1);
	}

	Listen(listenfd_ti, LISTENQ);

	FD_ZERO(&rset);
	for( ; ; ){

		FD_SET(listenfd_ec, &rset);
		FD_SET(listenfd_ti, &rset);
		maxfdp = max(listenfd_ec, listenfd_ti) + 1;
		Select(maxfdp, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd_ti, &rset)){

			connfd_ti = Accept( listenfd_ti, NULL ,NULL);
			if (fileflags2 = fcntl(connfd_ti, F_GETFL, 0) == -1)  {
		   		perror("fcntl F_GETFL");
			   	exit(1);
			}

			if (fcntl(connfd_ti, F_SETFL, (fileflags2 &  (~O_NONBLOCK)) == -1))  {
		   		perror("fcntl F_SETFL, O_NONBLOCK");
		   		exit(1);
			}
			printf("connection to time client established\n");
			fflush(stdout);
			Pthread_create(&tid, NULL, &time_echo, (void *)connfd_ti);
			FD_CLR(listenfd_ti, &rset);
		}

		if(FD_ISSET(listenfd_ec, &rset)){

			connfd_ec = Accept( listenfd_ec, NULL ,NULL);
			if (fileflags = fcntl(connfd_ec, F_GETFL, 0) == -1)  {
		   		perror("fcntl F_GETFL");
			   	exit(1);
			}

			if (fcntl(connfd_ec, F_SETFL, (fileflags &  (~O_NONBLOCK)) == -1))  {
		   		perror("fcntl F_SETFL, O_NONBLOCK");
		   		exit(1);
			}
			printf("connection to echo client established\n");
			Pthread_create(&tid, NULL, &str1_echo, (void *) connfd_ec);
			FD_CLR(listenfd_ec, &rset);
		}

		
	}
	exit(0);
}
