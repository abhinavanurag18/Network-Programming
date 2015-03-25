#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <string.h>
#include <linux/prctl.h>

struct nextpid {
	long mtype;
	pid_t next;
};

struct datamsg {
	long mtype;
	int k;
};

int prevk,flag = 1,msqid2,K;
pid_t next,parent;

void ksend(int,int);
int krcv(int);
void game();
void sig(int signo){
	if(signo == SIGUSR1){
		// printf("sigusr1 recvd %d\n",getpid() );
	}
}
int main(int argc, char **argv){
	/* Initializations */
	signal(SIGUSR1,sig);
	key_t key1,key2;
	int msqid1;
	int nextup = 0;
	K = atoi(argv[1]);
	char *cwd;
	char pathbuff[100];
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

    // prctl(PR_SET_PDEATHSIG, SIGHUP);
	/* getting the next pid */

    struct nextpid *m1;
	m1 = malloc(sizeof(struct nextpid));
	if(msgrcv(msqid1,m1,sizeof(struct nextpid)-sizeof(long),(long)getpid(),0) < 0){
		perror("msgrcv() failed for msqid1");
		exit(1);
	}
	next = m1->next;
	
	game();	
	
	

	/* executing the game after recieving the message */
	/* sending the messsage */
}

void ksend(int msqid,int k){
	struct datamsg *d2;
	d2 = malloc(sizeof(struct datamsg));

	d2->mtype = (long)next;
	d2->k = k;
	
	if(msgsnd(msqid,d2,sizeof(struct datamsg)-sizeof(long),0) == -1){
		perror("msgsnd() failed in ksend");
		exit(1);
	}
}

int krcv(int msqid){
	struct datamsg *d;
	d = malloc(sizeof(struct datamsg));
	int f;
	if((f = msgrcv(msqid,d,sizeof(struct datamsg)-sizeof(long),(long)getpid(),0)) < 0){
		// perror("msgrcv() failed in krcv game");
		
		exit(1);
		
	}
	
	return d->k;

}

void game(){		
		while(1){
			int k = krcv(msqid2);
			
			if(flag == 1){
				if(k == 0){
					printf("I am a foolish process %d, defeated.\n",getpid());
					flag = 0;
					prevk = k;
					ksend(msqid2,K);
					continue;

				}
				else if(k == prevk){
					printf("I am a truthful process %d, only the truth that always wins.\n",getpid());
					struct datamsg *d2;
					d2 = malloc(sizeof(struct datamsg));
					d2->mtype = (long)getppid();
					d2->k = k;
					int h;
					if((h = msgsnd(msqid2,d2,sizeof(struct datamsg)-sizeof(long),0)) == -1){
						perror("msgsnd() failed in ksend");
						
						exit(1);
					}
					
					exit(1);
				}
				else {
					prevk = k-1;
					printf("Process %d Recieved %d\n",getpid(),k);
					ksend(msqid2,k-1);
					continue;

				}
			
			}
			else {
				ksend(msqid2,k);

			}	
		}
			
	// }
	
}