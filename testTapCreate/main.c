#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <stdio.h>

static struct {
unsigned int flag;
const char *name;
} known_flags[] = {
{ IFF_TUN, "TUN" },
{ IFF_TAP, "TAP" },
{ IFF_NO_PI, "NO_PI" },
{ IFF_ONE_QUEUE, "ONE_QUEUE" },
};

int main()
{
    unsigned int features, i;
    
    int netfd = open("/dev/net/tun", O_RDWR);
    if (netfd < 0)
        err(1, "Opening /dev/net/tun");
    
    printf("%x\n", TUNGETFEATURES);
    
    if (ioctl(netfd, TUNGETFEATURES, &features) != 0) {
        printf("Kernel does not support TUNGETFEATURES, guessing\n");
        features = (IFF_TUN|IFF_TAP|IFF_NO_PI|IFF_ONE_QUEUE);
    }
    printf("Available features are: ");
    for (i = 0; i < sizeof(known_flags)/sizeof(known_flags[0]); i++) {
        if (features & known_flags[i].flag) {
            features &= ~known_flags[i].flag;
            printf("%s ", known_flags[i].name);
        }
        }
}
