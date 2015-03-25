#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>

extern int errno;
#define LISTENQ 5
#define MAX_BUF 10		/* Maximum bytes fetched by a single read() */
#define MAX_EVENTS 5		/* Maximum number of events to be returned from
				   a single epoll_wait() call */



struct msg
{
	long mtype;
	char mtext;
};

struct chat_message
{
	char from[20];
	char msg[200];
	struct chat_message *next; 
};

struct member
{
	int fd;
	char name[20];
	struct chat_message *next;
};



typedef struct msg msg;

struct member *memlist;

void errExit(char *s)
{
	perror(s);
	exit(1);
}

void update(struct member *memlist){
	int i = 0;
	for(i = 0;i<200;i++){
		memlist[i].fd = -2;
		memlist[i].next = NULL;
	}
}

int setnonblock(int fd){
	int blocking = 1;
	/* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) != -1;
}

void join(char *message,int fd,struct member *memlist){
	char *token;
	char name[20];
	token = strtok(message," ");
	token = strtok(NULL," ");
	int i = 0;
	for(i = 0;i<200;i++){
		if(memlist[i].fd == -2){
			memlist[i].fd = fd;
			strcpy(memlist[i].name,token);
			memlist[i].next = NULL;

			int x = write(memlist[i].fd,"You are added to the chatlist.\n",32);
			if(x < 0){
				perror("Write during join");
				exit(1);
			}
			break;

		}
	}
	
}


void list(char *message,int fd,struct member *memlist){
	printf("in: list\n");
	int i = 0;
	for(i = 0;i < 200; i++){
		if(memlist[i].fd > -1){
			if((write(fd,memlist[i].name,20)) < 0){
				perror("write in list.");
				exit(1);
			}
		}
		else {
			if(memlist[i].fd == -2){
				break;				
			}

		}
	}
}

void umsg(char *message, int fd, struct member *memlist){
	char *token;
	token = strtok(&message[5],"\r\n ");
	int i = 0;
	for(i = 0;memlist[i].fd > -2;i++){
		if(strcmp(token,memlist[i].name) == 0){
			token = strtok(NULL,"\r\n");
			// struct chat_message *umsg;
			// umsg = malloc((struct chat_message *)sizeof(struct chat_message));
			// umsg->from = "someone";
			// strcpy(umsg->msg,token);
			// if(memlist[i].next == NULL){
			// 	memlist[i].next = umsg;
			// }

			if((write(memlist[i].fd,token,strlen(token))) < 0){
				perror("umsg write error");
				exit(1);
			}
		}
	}

}

void bmsg(char *message, int fd,struct member *memlist){
	int i = 0;
	for(i = 0; memlist[i].fd > -2; i++){
		if((write(memlist[i].fd,&message[5],strlen(&message[5]))) < 0){
			perror("bmsg error");
			exit(1);
		}
	}
}

void leav(int fd,struct member *memlist){
	int i = 0;
	while(i<200){
		if(memlist[i].fd == fd){
			memlist[i].fd = -1;
			strcpy(memlist[i].name,"\0");
			memlist[i].next = NULL;
			write(fd,"You have left the chat server\n",30);
			shutdown(fd,SHUT_RDWR);
			break;
		}
	}
}


int parse(char *message,int fd, struct member *memlist){
	char msgcmd[5];
	memset(msgcmd,'\0',5);
	printf("msgcmd : %s\n",msgcmd);
	memcpy(msgcmd,message,4);
	printf("message : %s\n",message);
	printf("cmd : %s\n", msgcmd);
	if(strcmp(msgcmd,"JOIN") == 0){
		join(message,fd,memlist);
	}
	else if(strcmp(msgcmd,"LIST") == 0){
		list(message,fd,memlist);
	}
	else if(strcmp(msgcmd,"UMSG") == 0){
		umsg(message,fd,memlist);
	}
	else if(strcmp(msgcmd,"BMSG") == 0){
		bmsg(message,fd,memlist);
	}
	else if(strcmp(msgcmd,"LEAV") == 0){
		leav(fd,memlist);
	}
	else {
		printf("Unknown Command\n");
	}
	return 0;

}



void *processing_thread(void *fd1){
	struct member *ml;
	ml = (struct member *)memlist;
	int *fd = (int *)fd1; //cast the pointer into integer
	int msqid;
	key_t key;

	printf("%d\n",*fd);
	if ((key = ftok("eventdriven_chatserver.c", 'A')) == -1)
    {
      	perror ("ftok");
      	exit (1);
    }

  	if ((msqid = msgget (key, 0644 | IPC_CREAT)) == -1)
    {
      	perror("msgget");
      	exit(1);
    }
    
    printf("in: msq made\n");
    char message[200];
    int fd2;
    for(;;){
    	msg *buf;
		// printf("inside process\n");
		buf = malloc(sizeof(msg));
    	
    	// memset(buf->mtext,'\0',1);
		if(msgrcv(msqid,buf, sizeof(buf), *fd, 0) == -1)
		{
			perror ("msgrcv");
			exit (1);
		}
		fd2 = buf->mtype;
		if(buf->mtext == 'r'){
			if(recv(fd2,message,200,0) < 0){
				// perror("read from fd");
				// break;
				continue;
			}
			parse(message,fd2,ml);
		}
		free(buf);
    }
    
}






int main (int argc, char *argv[])
{

	
	memlist = malloc(200 * sizeof(struct member));
	update(memlist);


	int msqid;
	key_t key;
	int epfd, ready, fd, s, j, numOpenFds;
	struct epoll_event ev;
	struct epoll_event evlist[MAX_EVENTS];
	char buf[MAX_BUF];
	int listenfd, clilen;

	struct sockaddr_in cliaddr, servaddr;


	if ((key = ftok ("eventdriven_chatserver.c", 'A')) == -1)
    {
    	perror ("ftok");
    	exit (1);
    }

	if((msqid = msgget(key, IPC_CREAT | 0644)) == -1)
	{
    	perror ("msgget");
    	exit (1);
    }
	// printf("msg queue made\n");
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setnonblock(listenfd);
	bzero (&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind (listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
		perror("bind");
	// printf("socket made and bound\n");
	listen (listenfd, LISTENQ);

	if (argc < 2 || strcmp (argv[1], "--help") == 0)
		printf("Usage: %s <port>\n", argv[0]);

	

	

	epfd = epoll_create(20);
	if(epfd == -1)
		errExit ("epoll_create");


	ev.events = EPOLLIN;		/* Only interested in input events */
	ev.data.fd = listenfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
		errExit ("epoll_ctl");
	// printf("epoll made\n");

	
	

	while(1)
	{
		// printf("inside for\n");
		// printf("%d\n",MAX_EVENTS );
		ready = epoll_wait(epfd, evlist, MAX_EVENTS, 10);
		if(ready == -1)
		{
			if(errno == EINTR)
				continue;		/* Restart if interrupted by signal */
			else
				errExit ("epoll_wait");
		}
	  	// printf("nready=%d\n", ready);

		for (j = 0; j < ready; j++)
		{
			if (evlist[j].events & EPOLLIN)
	    	{
	    		if (evlist[j].data.fd == listenfd) {
					clilen = sizeof(cliaddr);
					char ip[128];
					memset (ip, '\0', 128);
					int connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
					setnonblock(connfd);
		  			if (cliaddr.sin_family == AF_INET){
				      	if(inet_ntop (AF_INET, &(cliaddr.sin_addr), ip, 128) == NULL)
							perror("Error in inet_ntop\n");
				    }

					if (cliaddr.sin_family == AF_INET6){
						inet_ntop(AF_INET6, &(cliaddr.sin_addr), ip, 128);
		    		}

		  			printf ("new client: %s, port %d\n", ip,ntohs(cliaddr.sin_port));

		  			ev.events = EPOLLIN;	
		  			ev.data.fd = connfd;
		  			if(epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
		    			errExit("epoll_ctl");
		    		printf("Added to epoll\n");

		    		pthread_t *tid;
					tid = malloc(sizeof(pthread_t));
					if(pthread_create(tid,NULL,processing_thread,(void *)&connfd)){
						perror("pthread");
						exit(1);
					}
					printf("Thread created\n");
					free(tid);

				}
	      		else {
	      			// printf("In: first else\n");
	      			msg *m1;
	      			m1 = malloc(sizeof(msg));
	      			// memset(m1->mtext,'\0',1);
	      			m1->mtype = (long)evlist[j].data.fd;
	      			m1->mtext = 'r';
		  			if(msgsnd(msqid,m1,sizeof(msg)-sizeof(long),0) == -1){
						perror("msgsnd()");
						exit(1);
					}
					printf("Msg sent : %ld\n",m1->mtype );
					free(m1);
				}
	    	}
	    	else {
	    		msg *m2;
      			m2 = malloc(sizeof(msg));
      			// memset(m2->mtext,'\0',1);
      			m2->mtype = (long)evlist[j].data.fd;
      			m2->mtext = 'w';
	  			if(msgsnd(msqid,m2,sizeof(msg)-sizeof(long),0) == -1){
					perror("msgsnd()");
					exit(1);
				}
	    	}
		}
	}
	if (msgctl(msqid, IPC_RMID, NULL) == -1) 
	{
		perror("msgctl");
		exit(1);
	}
}
