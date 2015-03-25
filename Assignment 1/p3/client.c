#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

int main(int argc,char **argv){

	if(argc != 3){
		printf("Incorrect usage.\nCorrect usage ./client [ip] [port]\n");
		exit(1);
	}


	int sd,flag = 0;
	char *hostname = argv[1];
	char ip[100];
	char cmd1[100],cmd2[100],cmd3[100],cmdf[100],c[100];
	struct sockaddr_in server;
	hostname_to_ip(hostname,ip);
	printf("IP: %s\n",ip);
	/* Create a socket */
	if((sd = socket(AF_INET,SOCK_STREAM,0)) < 0 )
		perror("socket() failed");

	/* Giving the address and port number to the socket where it has to connect */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr=inet_addr(ip);
	server.sin_port = htons(atoi(argv[2]));

	/* Connect the socket to the server */
	if((connect(sd, (struct sockaddr*) &server, sizeof(server))) < 0 ){
		perror("connect() failed");
		exit(EXIT_FAILURE);
	}
	printf("Connection Established\n");
	/* Send the command */
	while(1){
		printf("Enter the query string to be searched :- ");
		scanf("%s",cmd1);
		label1: printf("Do you want to search in the previously searched search result? (y/n)");
		scanf("%s",cmd2);
		if(!flag){
			if(strcmp(cmd2,"y") == 0){
				printf("No previous search result.\n");
				goto label1;
			}
		}
		strcpy(cmdf,cmd1);
		// printf("%s\n",cmdf );
		strcat(cmdf,"$");
		// printf("%s\n",cmdf );
		strcat(cmdf,cmd2);
		// printf("%s\n",cmdf );
		// strcat(cmdf,"$");
		// printf("%s\n",cmdf );
		
		if(strcmp(cmd2,"n") == 0){
			printf("Enter the filename to be searched from: ");
			scanf("%s",cmd3);
			// fflush(stdin);
			strcat(cmdf,"$");
			// printf("%s\n",cmdf );
			strcat(cmdf,cmd3);
			// printf("%s\n",cmdf );
		}
		// printf("flag1\n");
		// printf("%s\n",cmdf);
		// printf("flag2\n");
		send(sd,cmdf,strlen(cmdf),0);
		char resp[1000];
		if(recv(sd,resp,1000,0)== 0){
			perror("recv() failed");
			exit(EXIT_FAILURE);
		}
		resp[1000] = '\0';
		printf("Response : %s\n",resp );
		// sleep(1);
	}
	close(sd);
	exit(0);

	
}

int hostname_to_ip(char * hostname , char* ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
		
	if ( (he = gethostbyname( hostname ) ) == NULL) 
	{
		// get the host info
		herror("gethostbyname");
		return 1;
	}

	addr_list = (struct in_addr **) he->h_addr_list;
	
	for(i = 0; addr_list[i] != NULL; i++) 
	{
		//Return the first one;
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		return 0;
	}
	
	return 1;
}
