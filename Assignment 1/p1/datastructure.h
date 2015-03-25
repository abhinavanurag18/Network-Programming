#include<sys/types.h>
struct job
{
  int back;
  pid_t pid;
  int jid;
  int status;
  char command[100];
};

