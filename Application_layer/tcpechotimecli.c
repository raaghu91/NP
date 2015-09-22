#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#define SIZE 1024

int main(int argc,char *argv[])
{

        int option,child_pid,client_pipe[2];
	char buffer[SIZE],buf[SIZE],buf1[SIZE];
	int rc =0,stat;
	pid_t pid;

	struct in_addr **addr_list;
	struct in_addr inetaddr;
	struct hostent *he;

	
	if(argc != 2)
                err_quit("usage: client <IPaddress>/<hostname>");

	if(isalpha(argv[1][0])){
	
		if(( he = gethostbyname(argv[1])) == NULL) {
			herror("gethostbyname");
			return 2;
		}
		addr_list = (struct in_addr **) he->h_addr_list;
		printf("Connecting to server IP:%s\n", inet_ntoa(*addr_list[0]));
		argv[1]=inet_ntoa(*addr_list[0]);
	}
	else{
		inet_pton(AF_INET, argv[1], &inetaddr);
		if(( he = gethostbyaddr(&inetaddr, sizeof inetaddr, AF_INET)) == NULL){
			herror("gethostbyaddr");
			return 2;
		}
		printf("Connecting to server host name: %s\n", he->h_name);
	}

        while(1){
                printf("Enter a number:\n 1:tcp-echo 2:tcp-daytime 3:exit \n");
		fgets(buf,50,stdin);
		option=atoi(buf);
		if(pipe(client_pipe) == -1)
		{
			perror("pipe failed");
			exit(1);
		}
                switch(option){
                        case 1: 
                                if((child_pid=fork())==0)
                                {     
					close(client_pipe[0]);
					sprintf(buf1,"%d",client_pipe[1]);
					if(( execlp("xterm", "xterm", "-e", "./echo_cli",argv[1],buf1,(char *) 0)) < 0){
						perror("execing xterm:echo_cli failed\n");
						exit(1);
					}
				}
				if(child_pid == -1)
				{
					perror("fork error\n");
					exit(1);
				}

				close(client_pipe[1]);
				while(read(client_pipe[0],buffer,SIZE) != 0)
					printf("Message from echo_cli: %s",buffer);
		                close(client_pipe[0]);
				pid=wait(&stat);
				printf("child xterm:echo_cli terminated\n");
			
				break;
			case 2:
				if((child_pid=fork())==0)
                                {       
					close(client_pipe[0]);
					sprintf(buf1,"%d",client_pipe[1]);
					if(( execlp("xterm", "xterm", "-e", "./time_cli",argv[1],buf1,(char *) 0)) < 0){
						perror("execing xterm:time_cli failed\n");
						exit(1);
					}
                                }
				if(child_pid == -1)
				{
					perror("fork error\n");
					exit(1);
				}
				close(client_pipe[1]);
				while(read(client_pipe[0],buffer,SIZE) != 0)
					printf("Message from time_cli: %s",buffer);
		                close(client_pipe[0]);
				pid=wait(&stat);
				printf("child xterm:time_cli terminated\n");
				break;
			case 3:
				exit(0);
			default:printf("Please enter correct option\n");
				break;

		}
	}
} 
