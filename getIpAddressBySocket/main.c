#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_link.h>
#include <errno.h>
#include <stdlib.h>
#include "main.h"
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdlib.h>

static int getIp4Address(char *iface_name, int *ip_addr, int *mask)
{
    int sockfd = -1;
    struct ifreq ifr;
    struct sockaddr_in *addr = NULL;
	struct sockaddr_in *sin;

    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, iface_name);
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket error!\n");
        return -1;
    }
    
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
		*ip_addr = addr->sin_addr.s_addr;
	}
    
	if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) == 0) {
        sin = (struct sockaddr_in *)&ifr.ifr_netmask;
		*mask = sin->sin_addr.s_addr;
		close(sockfd);
        return 0;
    }

    close(sockfd);

    return -1;
}


int main(int argc, char **argv) {
	int Ipaddr = 0, mask = 0;

	getIp4Address("wlp5s0", &Ipaddr, &mask);

	printf("wlp5s0 ip addr: 0x%x, mask: 0x%x\n", Ipaddr, mask);
	return 0;
}
