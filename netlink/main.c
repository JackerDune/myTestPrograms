#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_link.h>
#include <errno.h>
#include <stdlib.h>
#include "main.h"

#define SHELL_CMD(X) "grep -Fw "X" ./test.org | awk '{print $2}'"

char* shellcmd(char* cmd, char* buff, int size)
{
  char temp[256];
  FILE* fp = NULL;
  int offset = 0;
  int len;
   
  fp = popen(cmd, "r");
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
    pclose(fp);
  }
 
  return buff;
}

void getIpaddress(char *ifname, char *Buff, int len) 
{
	char command[256] = {0};

	sprintf(command, "ip addr show %s |grep global |awk '{print $2}'", ifname);
	shellcmd(command, Buff, 64);
	printf("command: %s\n", command);
	printf("Buff: %s\n", Buff);
}

void delIpaddress(char *ifname, char *ip) 
{
	char command[256] = {0};
	char buff[64] = {0};

	sprintf(command, "ip addr del %s dev %s", ip, ifname);
	shellcmd(command, buff, 64);
	printf("command: %s\n", command);
	printf("buff: %s\n", buff);
}


void addIpaddress(char *ifname, char *ip) 
{
	char command[256] = {0};
	char buff[64] = {0};

	sprintf(command, "ip addr add %s dev %s", ip, ifname);
	shellcmd(command, buff, 64);
	printf("command: %s\n", command);
	printf("buff: %s\n", buff);
}

void reloadIpaddress(char *ifname) 
{
	char Ipaddr[64] = {0};
	
	getIpaddress(ifname, Ipaddr, 64);
	if (strlen(Ipaddr) > 4) {
		Ipaddr[strlen(Ipaddr)-1] = '\0';
		delIpaddress(ifname, Ipaddr);
		addIpaddress(ifname, Ipaddr);
	}
}


int main(int argc, char **argv) {
	char buff[1024];
	char name[10] = {"vrf1"};
	char command[256] = {0};
	char Ipaddr[64] = {0};

	memset(buff, 0, sizeof(buff));
	sprintf(command, "grep -Fw " "%s" " ./test.org | awk '{print $2}'", name);
	printf("%s\n", command);
	printf("%d\n", atoi(shellcmd(command, buff, sizeof(buff))));

	reloadIpaddress("br100");

	return 0;
}
