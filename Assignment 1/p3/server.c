#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CMDBUFFSIZE 100




int main(int argc,char **argv){
	int sd,psd,status;
	struct sockaddr_in name,client;
	int clilen,cmdrecv;
	int port;
	pid_t ret;
	char cmdbuff[CMDBUFFSIZE];
	clilen = sizeof(client);

	if(argc != 2){
		printf("Incorrect usage\n.Correct usage ./server [port]\n");
		exit(1);
	}
	else{
		port = atoi(argv[1]);
	}

/* Create a socket */
	if((sd = socket(AF_INET,SOCK_STREAM,0)) < 0)
		perror("socket() failed");
/* Define local ip and port to listen to clients */
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
/* Bind socket address to socket using socket file descriptor */
	if((bind(sd,(struct sockaddr *)&name, sizeof(name))) != 0){
		perror("bind() failed");
		exit(1);
	}
	printf("Port bound to %d\n",port);

/* Making passive socket open using listen() */
	if(listen(sd,SOMAXCONN) != 0){
		perror("listen() failed");
		exit(1);
	}
	printf("Server waiting for clients.\n");
	// int i = 2;
/* accepting the topmost SYN request queued on the socket fd */
	while(1){
		if((psd = accept(sd,(struct sockaddr *)&client,&clilen)) < 0)
			perror("accept() failed");

		printf("Server handling the client");
/*--------------- Making Concurrent Server -------------*/
	/* forking to create a child process to handle this connection between server and a specific client which got connected.*/
		if((ret = fork()) == 0){
		/* -------------- In Child Process -------------------- */
			printf("In Child Level 1\n");
			/*  Closing active socket fd in child process */
			close(sd);
			/* Create a FIFO/PIPE for storing the search result */
			int p[2],perr[2];
			char *token, delim[2] = "$";
			char resp[1000];
			if(pipe(p) == -1)
				perror("pipe() fails");
			if(pipe(perr) == -1)
				perror("pipe() fails");

			
			/* Read the socket buffer and transfer the data in process buffer recursively */
			
			while(1){
				memset(cmdbuff,'\0',CMDBUFFSIZE);
				printf("Waiting for request ...\n");
				if((cmdrecv = recv(psd,cmdbuff,CMDBUFFSIZE,0)) == 0){
					perror("recv() failed");
					exit(0);
				}
				printf("Request string : %s\n",cmdbuff );
				/* Parse the request */
				char query[100] = "";
				char yn[2] = "";
				char file[1000] = "";
				token = strtok(cmdbuff,delim);
				int i = 1;
				
				while(token != NULL){
					switch(i){
						case 1:
							strcpy(query,token);
							break;
						case 2:
							strcpy(yn,token);
							break;
						case 3:
							strcpy(file,token);
							break;
						default: break;
					}
					
					token = strtok(NULL,delim);
					i++;
				}
				printf("Request string tokenised : %s %s %s\n",query,yn,file);
				int p2[2];
				if(pipe(p2) == 0){
					write(perr[1],"1",1);
					pid_t ret1 = fork();
					if(ret1 == (pid_t)0){
						printf("In Child Level 2\n");
						close(1);
						dup(p2[1]);
						dup2(p[0],0);
						dup2(perr[1],STDERR_FILENO);
						/* if Y, search the query string in the FIFO/PIPE */
						if(strcmp(yn,"y") == 0){
							execlp("grep","grep",query,(char *)NULL);
							exit(EXIT_FAILURE);
						}
						/* if N, grep the search query in the specified file and write the response in the socket buffer or an another PIPE/FIFO */
						else{
							execlp("grep","grep",query,file,(char *)NULL);	
							exit(EXIT_FAILURE);
						}
							
					}
					else{
						/* send the data in socket buffer or outPIPE/FIFO to the client. */
						/* wait for the next request query */
						wait(ret1);
						close(p2[1]);
						// close(p[0]);

						printf("sending response\n");
						char errmsg[1000];
						memset(errmsg,'\0',1000);
						int errbytes = read(perr[0],errmsg,1000);
						printf("Err bytes %d\n Err msg : %s\n",errbytes,errmsg);
						memset(resp,'\0',1000);
						if((errbytes > 0) && (strcmp(errmsg,"1") != 0)){
							printf("Response returned : %s\n",errmsg);
							send(psd,errmsg,1000,0);
						}
						else {
							
							int datap = read(p2[0],resp,1000);
							printf("Data bytes : %d\n",datap );
							write(p[1],resp,strlen(resp));
							close(p[1]);
							printf("Response returned : %s\n",resp);
							send(psd,resp,1000,0);	
						}
						
		
					}
				}
				
				
			}
			
			close(psd);
			
			
			
			exit(EXIT_SUCCESS);
			
		}
		
		/* -------------- In Parent Process ------------------- */
			/* Close passive socket fd in parent process */
			close(psd);
			// exit(0);

	}


	 


	


	
}
