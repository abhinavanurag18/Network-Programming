/************************************************************************************
* Network Programming Assingment 2
* Ravi Shankar Pandey
* (2012C6PS676P)
* Abhinav Anurag
* (2012C6PS523P)
* Aditi Agarwal
* (2012C6PS510P)
************************************************************************************/

/*includes*/
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

/*user data stucture*/
struct user_data
{
	char name[50];
	int isReg;
	int connfd;
};

/*global variable*/
struct user_data *clients;
int listenfd, nthreads;
socklen_t clilen;
struct sockaddr_in cliaddr, servaddr;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t slock = PTHREAD_MUTEX_INITIALIZER;

/*function prototypes*/
void init_clients();
void err4(int connfd);
void err3(int connfd);
void err2(int connfd);
void err1(int connfd);
int uni_msg(int connfd, int index, char *name);
int leave(int connfd, int index);
int broadcast(int connfd, int index, char* msg);
int list(int connfd);
int register_user(int connfd, int index);
int bmsg(int connfd, char* name, int index, char* msg);
struct user_data getData(int connfd);

/*main*/
int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("Improper format %s [port] [N].\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	int i;
	void sig_int(int), thread_make(int);

	nthreads = atoi(argv[2]);
	clients = (struct user_data *)malloc(sizeof(struct user_data)*nthreads);
	init_clients();

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1)
	{
		perror("listenfd");
		exit(EXIT_FAILURE);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if(listen(listenfd, 2*nthreads) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	for(i=0; i<nthreads;i++)
		thread_make(i);

	/*all the work done in thread*/
	for(;;)
		pause();
}

/*creates thread*/
void thread_make(int i)
{
	void *thread_main(void *);
	pthread_t thread_tid;
	pthread_create(&thread_tid, NULL, &thread_main, (void *)i);
	return;
}

/*thread main function*/
void *thread_main(void *arg)
{
	int connfd;
	void web_client(int, int);
	socklen_t clilen;
	struct sockaddr_in *cliaddr;
	cliaddr = malloc(sizeof(servaddr));
	if(cliaddr == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	//printf("thread %d starting\n", (int) arg);
	for(;;)
	{
		clilen = sizeof(cliaddr);
		pthread_mutex_lock(&mlock);
		connfd = accept(listenfd, (struct sockaddr*)cliaddr, &clilen);
		if(connfd < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		//printf("new client: %s, port %d\n", inet_ntop(AF_INET, cliaddr->sin_addr, 4, NULL), ntohs(cliaddr->sin_port));
		pthread_mutex_unlock(&mlock);
		web_client(connfd, (int)arg);
		close(connfd);
	}
}

/*Handles the client requests*/
void web_client(int connfd, int index)
{
	char recv_buff[201];
	char *reply = "username not registered.\n";

	if(register_user(connfd, index))
	{
		pthread_mutex_lock(&slock);
		char * name = clients[index].name;
		pthread_mutex_unlock(&slock);
		while(1)
		{
			memset(recv_buff, '\0', 201);
			if(read(connfd, recv_buff, 200) < 0)
			{
				perror("read");
				exit(EXIT_FAILURE);
			}
			int len = strlen(recv_buff);
			if(!((int)recv_buff[len-1] == 10 && 
				(int) recv_buff[len-2] == 13))
			{
				err1(connfd);
				continue;
			}
			recv_buff[len-2] = '\0';
			if(strncmp("LEAV", recv_buff, 4) == 0)
			{
				leave(connfd, index);
				break;
			}
			else if(strncmp("JOIN", recv_buff, 4) == 0)
			{
				err4(connfd);
			}
			else if(strncmp("LIST", recv_buff, 4) == 0)
			{
				list(connfd);
			}
			else if(strncmp("UMSG ", recv_buff, 5) == 0)
			{
				uni_msg(connfd, index, &recv_buff[5]);
			}
			else if(strncmp("BMSG ", recv_buff, 5) == 0)
			{
				broadcast(connfd, index, &recv_buff[5]);
			}
			else
			{
				err3(connfd);
			}
		}
	}
	shutdown(connfd, 2);
}

/*uincating message*/
int uni_msg(int connfd, int index, char *name)
{
	char recv_buff[201];
	char send_data[251];
	char *err_msg = "ERROR <not online>\n";
	struct user_data a = getData(connfd);

	memset(recv_buff, 0, 201);
	memset(send_data, 0, 251);
	if (read(connfd, recv_buff, 200) < 0)
	{
		err2(connfd);
		return 0;
	}
	int j,flag=0,len = strlen(recv_buff);
	recv_buff[len-2] = '\0';
	pthread_mutex_lock(&slock);
	for(j=0;j<nthreads;j++)
	{
		if(strncmp(name, clients[j].name, strlen(name)) == 0)
		{
			sprintf(send_data, "[%s]:- %s\n",clients[index].name, recv_buff);
			printf("%s\n",send_data);
			if ( write(clients[j].connfd, send_data,
				 strlen(send_data)) < 0)
			{
				err2(connfd);
			}
			flag = 1;
		}
		if(flag)
			break;
	}
	pthread_mutex_unlock(&slock);
	if(!flag)
	{
		write(connfd, err_msg, strlen(err_msg));
	}
}

/*get user details */
struct user_data getData(int connfd)
{
	int j;
	pthread_mutex_lock(&slock);
	for(j=0;j<nthreads;j++)
	{
		if(connfd == clients[j].connfd)
		{
			pthread_mutex_unlock(&slock);
			return clients[j];
		}
	}
	pthread_mutex_unlock(&slock);
	struct user_data a;
	a.connfd = -1;
	a.isReg = 0;
	return a;
}

/*broadcasting message*/
int broadcast(int connfd, int index, char* msg)
{
	int j;
	char info[300];
	pthread_mutex_lock(&slock);
	sprintf(info, "[%s]:- %s\n",clients[index].name, msg);
	for(j=0;j<nthreads;j++)
	{
		if(j==index || clients[j].isReg == 0)
		{
			continue;
		}
		write(clients[j].connfd, info, strlen(info));
	}
	pthread_mutex_unlock(&slock);
	return 1;
}

/*listing all the connected clients*/
int list(int connfd)
{
	char *buffer;
	buffer = malloc(20*nthreads*sizeof(char));
	int j = 0,temp;
	memset(buffer, '\0', 20);
	pthread_mutex_lock(&slock);
	for(j=0;j<nthreads;j++)
	{
		if(clients[j].isReg)
		{
			temp = strlen(buffer);
			sprintf(&buffer[temp], "%s\n", clients[j].name);
		}
	}
	temp = strlen(buffer);
	sprintf(&buffer[temp], "\n");
	pthread_mutex_unlock(&slock);
	if(write(connfd, buffer, strlen(buffer)) < 0)
	{
		err2(connfd);
		return 0;
	}
	free(buffer);
	return 1;
}

/*exiting*/
int leave(int connfd, int index)
{
	pthread_mutex_lock(&slock);
	strcpy(clients[index].name, "");
	clients[index].isReg = 0;
	clients[index].connfd = -1;
	pthread_mutex_unlock(&slock);
	char *msg = "user connection terminated.\n";
	write(connfd, msg, strlen(msg));
	return 1;
}

/*Return 1 on success else 0*/
int register_user(int connfd, int index)
{
	char buffer[201];
	char * suc_msg = "logged in\n";
	char * fail_msg = "invalid command\n";
	if(read(connfd, buffer, 200) < 0)
	{
		perror("read");
		return 0;
	}
	if(strncmp("JOIN", buffer, 4) == 0)
	{
		if( write(connfd, suc_msg, strlen(suc_msg)) < 0)
		{
			perror("send");
			return 0;
		}
		pthread_mutex_lock(&slock);
		strcpy(clients[index].name, &buffer[5]);
		clients[index].name[strlen(clients[index].name) - 2] = '\0';
		clients[index].isReg = 1;
		clients[index].connfd = connfd;
		pthread_mutex_unlock(&slock);
		return 1;
	}
	write(connfd, fail_msg, strlen(fail_msg));
	pthread_mutex_lock(&slock);
	clients[index].isReg = 0;
	pthread_mutex_unlock(&slock);
	return 0;
}

/*initializing all the clients array to null*/
void init_clients()
{
	int j;
	for(j=0;j<nthreads;j++)
	{
		clients[j].isReg = 0;
		clients[j].connfd = -1;
		strcpy(clients[j].name, "");
	}
}

/*error messages*/
void err4(int connfd)
{
	char *err4 = "Cannot have multiple logins.\n";
	if(write(connfd, err4, strlen(err4))<0)
	{
		perror("write");
	}
}
void err3(int connfd)
{
	char *err3 = "Unknown Command.\n";
	if(write(connfd, err3, strlen(err3))<0)
	{
		perror("write");
	}
}
void err2(int connfd)
{
	char *err2 = "Unable to process query.\n";
	if(write(connfd, err2, strlen(err2))<0)
	{
		perror("write");
	}
}

void err1(int connfd)
{
	char *err1 = "Improper format.\n";
	if(write(connfd, err1, strlen(err1))<0)
	{
		perror("write");
	}
}
