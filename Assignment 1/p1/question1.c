#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include"datastructure.h"
#include<signal.h>
#define MAX 500
void handle_signal_sigint(int);
void  handle_signal_sigint(int signo)
{ printf("\n >");

}
void  handle_signal_sigtstp()
{
// kill(getpid(),SIGKILL);
}

int main(int argc, char **argv, char* envp[])
{
  struct job joblist[MAX];
  char *input=(char *)malloc(sizeof(char) * MAX);
  char * Parameters[50];
  int index_p  = 0;  int i = 0;int jobs=0;
  char* token;
  char command[50]="/bin/";
  pid_t pid;
  int status,j;
  int flag=0,bgflag=0;int jid=0;
  char* temp;
 // signal(SIGTSTP,SIG_IGN);
 // signal(SIGTSTP, handle_signal_sigtstp);
 signal(SIGINT, SIG_IGN);
  signal(SIGINT, handle_signal_sigint);
  // Initialisations done
  for(i=0;i<MAX;i++)
  {
        joblist[i].jid=0;
        joblist[i].pid=0;
        joblist[i].status=0;
        strcpy(joblist[i].command," ");
  }
  while(1)
  {jid++;
  memset(input,0,MAX);
memset(Parameters,0,50);//input and parameters initialised.
  index_p=0;
  printf(">");
  fgets(input,MAX-1, stdin);//input from user
  if(strstr(input,"fg")!=NULL |strstr(input,"kill")!=NULL|strstr(input,"bg")!=NULL |strstr(input,"jobs")!=NULL)
  {
    flag=1;
   // printf("jobs flag set");

  }//set flag for future reference
 // printf("reached here");
  token = strtok(input, "  \t\n");

  Parameters[index_p] = token;
  index_p++;
  if(token==NULL)
  continue;

  while(token != NULL)
    {     //  printf("toekns being made");

      token = strtok(NULL, " \t\n");
      if(flag==1)
       { // printf("flag set to 1");
          if(token!=NULL){
          flag=atoi(token);

         // printf("Anothr flag set");
         }
          else
        { flag=-1; }
       }
      Parameters[index_p] = token;
      index_p++;

    }
  char* last=Parameters[index_p-2];
  for(i=0;i<strlen(last);i++)
  {last=last+1;}
  Parameters[index_p]=NULL;
  last=strstr(Parameters[index_p-2],"&");
  if(last!=NULL) {*last='\0';bgflag=1;}
  strcat(command,Parameters[0]);
  pid=fork();//new process is forked.
  if(pid>0)
 {   if(flag>0|flag==-1)
        {//printf("reached here");
                if(strstr(command,"fg")!=NULL)
                        {
                                for(i=0;i<jid;i++)
                                {
                                        if(joblist[i].jid==flag)
                                                {
                                                        kill(joblist[i].pid,SIGCONT);
                                                        break;
                                                }

                               }
                        }  //end of elseif
               else if(strstr(command,"jobs")!=NULL)
                        {       //printf("yahan pahunch gye");/*
                                for(i=0;i<jid;i++)
                                {
                                        if(joblist[i].back==1)
                                                printf("jid :%d command: %s  processid :%d status: %d",joblist[i].jid,joblist[i].command,joblist[i].pid,joblist[i].status);
                                }
                        }//end of elseif
              else if(strstr(command,"bg")!=NULL)
                        {
                                joblist[jid].jid=flag;
                                joblist[jid].pid=pid;
                                jid++;
                                bgflag=1;
                                kill(pid,SIGCONT);
                        }//end of elseif
             else if(strstr(command,"kill")!=NULL)
                        {
                                for(i=0;i<jid;i++)
                                {
                                        if(joblist[i].jid==flag)
                                        {
                                                if(joblist[i].pid>0)
                                                        {
                                                                kill(joblist[i].pid,SIGKILL);
                                                                for(j=i+1;j<jid;j++)
                                                                {
                                                                        joblist[j-1].jid=joblist[j].jid;
                                                                        joblist[j-1].pid=joblist[j].pid;
                                                                        strcpy(joblist[j-1].command,joblist[j].command);
                                                                                                                                                   }
                                                        //jid--;
                                                        }

                                                else
                                                        printf("Trying to kill all processes");
                                                break;
                                        }
                                }
                       }//end of elsif


         }


                 if(jid>=MAX){printf("Maximum limit of jobs exeeded ...");exit(1);  }
                 joblist[jid].jid=jid;
                 joblist[jid].pid=pid;
                strcpy(joblist[jid].command,command);
                joblist[i].status=0;
                // printf("pid is %d,%d",getpid(),pid);
                 if(last!=NULL)
                        {       if(*last=='&'){
                                bgflag=1;
                                joblist[jid].back=1;}
                                // printf("corrected command is %s",command);
                        }


  }//end of pid>0
  else if(pid==0)
  {
        if(flag>0 |flag==-1){exit(0);}
       // printf("%s",command);
       // if( bgflag==1){last=strstr(command,"&");*last='\0';}
        execve(command, Parameters,envp);
         printf("Your command FAILED!\n");
         exit(0);
 }
  else if(pid==-1)
  perror("fork");
  if(!bgflag)
  wait(&status);
  else
    {
   pid = waitpid (WAIT_ANY, &status, WUNTRACED|WNOHANG);
  }

  int flag2=0;
 // printf("parent retrns");

  for(i=0;i<jid;i++)
  {
      if(joblist[i].pid==pid)
      {
      flag2=1;
      break;
      }
  }
//  printf("status %d",status);
  if(flag2)
  { joblist[i].status=WEXITSTATUS(status);
        /*if (WIFSTOPPED (status))
    strcpy(joblist[i].status,"stopped");
    if(WIFSIGNALED (status))
     strcpy(joblist[i].status,"Terminated by signal");*/
    if(WIFEXITED(status))
      { joblist[i].status=1;;for(j=i+1;j<jid;j++)
                                                                {
                                                                        joblist[j-1].jid=joblist[j].jid;
                                                                        joblist[j-1].pid=joblist[j].pid;
                                                                        joblist[j].jid=0;
                                                                        joblist[j].pid=0;
                                                                        strcpy(joblist[j-1].command,joblist[j].command);
                                                                       joblist[j-1].status=joblist[j].status;
                                                                }


  }}//end of status fixing
 if(flag==0)
  printf("jobid: %d pid :%d \n",joblist[i].jid,joblist[i].pid);
  flag=0;bgflag=0;
          strcpy(command,"/bin/");

}//while loop over
return 0;
}
                      