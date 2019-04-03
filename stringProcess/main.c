#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define MAX_NAME_LEN 32 
#define MAX_BUFFER_LEN 256


static long openmax = 0;
//#endif
 
/*
 * If OPEN_MAX is indeterminate, we're not
 * guaranteed that this is adequate.
 */
#define OPEN_MAX_GUESS 1024
 
long open_max(void)
{
    if (openmax == 0) {      /* first time through */
        errno = 0;
        if ((openmax = sysconf(FOPEN_MAX)) < 0) {
           if (errno == 0)
               openmax = OPEN_MAX_GUESS;    /* it's indeterminate */
           else
               printf("sysconf error for _SC_OPEN_MAX");
        }
    }
 
    return(openmax);
}
 
static pid_t    *childpid = NULL;  /* ptr to array allocated at run-time */
static int      maxfd;  /* from our open_max(), {Prog openmax} */
 
FILE *vpopen(const char* cmdstring, const char *type)
{
	int pfd[2];
	FILE *fp;
	pid_t	pid;
 
	if((type[0]!='r' && type[0]!='w')||type[1]!=0)
	{
		errno = -11;
		return(NULL);
	}
 
    if (childpid == NULL) {     /* first time through */  
                /* allocate zeroed out array for child pids */  
        maxfd = open_max();  
        if ( (childpid = (pid_t *)calloc(maxfd, sizeof(pid_t))) == NULL)  
            return(NULL);  
    }
 
	if(pipe(pfd)!=0)
	{
		return NULL;
	}
	
	if((pid = vfork())<0)
	{
		return(NULL);   /* errno set by fork() */  
	}
	else if (pid == 0) {	/* child */
		if (*type == 'r')
		{
			close(pfd[0]);  
            if (pfd[1] != STDOUT_FILENO) {  
                dup2(pfd[1], STDOUT_FILENO);  
                close(pfd[1]);  
            } 			
		}
		else
		{
            close(pfd[1]);  
            if (pfd[0] != STDIN_FILENO) {  
                dup2(pfd[0], STDIN_FILENO);  
                close(pfd[0]);  
            }  			
		}
 
		/* close all descriptors in childpid[] */  
		for (int i = 0; i < maxfd; i++)  
		if (childpid[ i ] > 0)	
			close(i);  
 
        execl("/bin/sh", "sh", "-c", cmdstring, (char *) 0);  
        _exit(127);		
	}
 
    if (*type == 'r') {  
        close(pfd[1]);  
        if ( (fp = fdopen(pfd[0], type)) == NULL)  
            return(NULL);  
    } else {  
        close(pfd[0]);  
        if ( (fp = fdopen(pfd[1], type)) == NULL)  
            return(NULL);  
    }
 
    childpid[fileno(fp)] = pid; /* remember child pid for this fd */  
    return(fp);  	
}
 
 
int vpclose(FILE *fp)
{
    int     fd, stat;  
    pid_t   pid;  
  
    if (childpid == NULL)  
        return(-1);     /* popen() has never been called */  
  
    fd = fileno(fp);  
    if ( (pid = childpid[fd]) == 0)  
        return(-1);     /* fp wasn't opened by popen() */  
  
    childpid[fd] = 0;  
    if (fclose(fp) == EOF)  
        return(-1);  
  
    while (waitpid(pid, &stat, 0) < 0)  
        if (errno != EINTR)  
            return(-1); /* error other than EINTR from waitpid() */  
  
    return(stat);   /* return child's termination status */  
}

char* runshellcmd(char* cmd, char* buff, int size)
{
  char temp[MAX_BUFFER_LEN];
  FILE* fp = NULL;
  int offset = 0;
  int len;

  fp = vpopen(cmd, "r");
  if(fp == NULL)
  {
    return NULL;
  }

  while(fgets(temp, sizeof(temp), fp) != NULL)
  {
    len = strlen(temp);
    if(offset + len < size)
    {
      strcpy(buff+offset, temp);
      offset += len;
    }
    else
    {
      buff[offset] = 0;
      break;
    }
  }

  if(fp != NULL)
  {
    vpclose(fp);
  }

  return buff;
}

void get_vxlan_local_ip_by_name(char *name)
{
	char Buffer[MAX_NAME_LEN] = {0};
	char Ip[MAX_NAME_LEN] = {0};
	char command[MAX_BUFFER_LEN] = {0};
	int vni;
	struct in_addr addr;
	
	sprintf(command, "ip -d link show %s | grep local| awk '{print $3,$5}'", name);
	runshellcmd(command, Buffer, sizeof(Buffer));
	printf ("Buffer: %s\n", Buffer);
	if (strlen(Buffer) != 0) {
		sscanf(Buffer, "%d %s", &vni, Ip);	
	}

	printf("Ip: %s\n", Ip);
	inet_aton((const char *)Ip, &addr);
	printf("vxlan %d, local ip:0x%x\n", vni, addr.s_addr);
	//return id;
}


int main()
{
	get_vxlan_local_ip_by_name("vxlan100");
	return 0;
}
