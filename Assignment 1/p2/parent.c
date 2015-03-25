#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <linux/prctl.h>

struct nextpid {
	long mtype;
	pid_t next;
};

struct datamsg {
	long mtype;
	int k;
};

void forkchild(pid_t *,int,char *);
void sig(int);
void ksend(int,pid_t,int);
int krcv(int);


int main(int argc, char **argv){
	/* Initializations */
	key_t key1,key2;
	int msqid1,msqid2;
	int n,k;
	char *cwd;
	char pathbuff[100];
	pid_t *pidarray;
	pidarray = malloc((n+1)*sizeof(pid_t));

	if(argc != 3){
		printf("Incorrect Usage.Correct Usage : ./p2 N K where N < K.\n");
		exit(1);
	}
	else if(atoi(argv[1]) > atoi(argv[2])){
		printf("Incorrect Usage.Correct Usage : ./p2 N K where N < K.\n");
		exit(1);
	}
	else {
		n = atoi(argv[1]);
		k = atoi(argv[2]);
	}


	/* Making msqids */
	getcwd(pathbuff,100);
	strcat(pathbuff,"/parent.c");
	if((key1 = ftok(pathbuff,'e')) == -1){
		perror("ftok() error");
		exit(1);
	}
	if ((msqid1 = msgget (key1, 0644 | IPC_CREAT)) == -1){
    	perror ("msgget");
    	exit (1);
    }
    if((key2 = ftok(pathbuff,'f')) == -1){
		perror("ftok() error");
		exit(1);
	}	
    if ((msqid2 = msgget (key2, 0644 | IPC_CREAT)) == -1){
    	perror ("msgget");
    	exit (1);
    }
	/* making n children and loading them with game.c */
    forkchild(pidarray,n,argv[2]);
    
    int i = 0;
    while(i < n){
    	struct nextpid *n1;
    	n1 = malloc(sizeof(struct nextpid));
    	n1->mtype = (long)pidarray[i];
    	n1->next = pidarray[(i+1)%n];
    	// printf("n : mtype %ld next %d\n",n->mtype,n->next );
    	i++;
    	if(msgsnd(msqid1,n1,sizeof(struct nextpid)-sizeof(long),0) == -1){
    		perror("msgsnd() failed 2");
    		exit(1);
    	}

    }
    

	/* Sending sigusr1 to first child */
	int u = 0;

	// while(u < n){
	// 	kill(pidarray[u],SIGUSR1);
	// 	u++;	
	// }
	kill(pidarray[0],SIGUSR1);
	ksend(msqid2,pidarray[0],k);
	/* Waiting for msg when game ends */
	struct nextpid *m2;
	m2 = malloc(sizeof(struct nextpid));
	while(msgrcv(msqid2,m2,sizeof(struct nextpid)-sizeof(long),(long)getpid(),0)  < 0){
		perror("msgrcv() failed for msqid1 1");
		exit(1);
	}
	printf("game ended\n");
	/* msgctl all the message queues */
	if (msgctl(msqid1, IPC_RMID, NULL) == -1) 
	{
		perror("msgctl 1");
		exit(1);
	}	
	if (msgctl(msqid2, IPC_RMID, NULL) == -1) 
	{
		perror("msgctl 2");
		exit(1);
	}

	return(EXIT_SUCCESS);
}

void forkchild(pid_t *pidarray,int n,char *k){
	pid_t ret,i = 0;
	
	while(i < n){
		ret = fork();
		if(ret < 0){
			strerror(errno);
		}
		else if(ret == 0){
			signal(SIGUSR1,sig);
			if(i == 0)
				pause();
			if(execlp("./game","./game",k,NULL) == -1){
				perror("exec error");
				exit(1);
			}

		}
		else {
			pidarray[i] = ret;
			i++;

		}

	}
	

}

void sig(int signo){
	if(signo == SIGUSR1){
		// printf("got sigusr1 %d \n",getpid());
	}
}

void ksend(int msqid,pid_t rec,int k){
	struct datamsg *d2;
	d2 = malloc(sizeof(struct datamsg));
	d2->mtype = (long)rec;
	d2->k = k;
	if(msgsnd(msqid,d2,sizeof(struct datamsg)-sizeof(long),0) == -1){
		perror("msgsnd() failed in ksend");
		exit(1);
	}
}

int krcv(int msqid){
	struct datamsg *d;
	d = malloc(sizeof(struct datamsg));
	while(msgrcv(msqid,d,sizeof(struct datamsg)-sizeof(long),(long)getpid(),0) < 0){
		perror("msgrcv() failed in krcv parent");
		exit(1);
	}
	return d->k;

}