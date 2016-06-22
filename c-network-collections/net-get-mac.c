/**
 * @file net-get-mac.c
 * @brief Get mac address by interface name
 * @author Jianjiao Sun <jianjiaosun@163.com>
 * @version 1.0
 * @date 2016-05-23
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>  

#define     NET_MAC_LENGTH  18

int get_mac(char *if_name, char *mac, int len)   
{
	struct ifreq ifreq;
	int sock;
    int ret = -1;
    int name_len;

    if (len < NET_MAC_LENGTH)
    {
        return -1;
    }

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

    name_len = sizeof(ifreq.ifr_name);
	strncpy(ifreq.ifr_name, if_name, name_len);  
    ifreq.ifr_name[name_len - 1] = '\0';

	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) >= 0)
	{
        snprintf (mac, len, "%02X:%02X:%02X:%02X:%02X:%02X", 
            (unsigned char) ifreq.ifr_hwaddr.sa_data[0], 
            (unsigned char) ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned char) ifreq.ifr_hwaddr.sa_data[2],
            (unsigned char) ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned char) ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);

        ret = 0;
	}

    close(sock);

    return ret;
}

int main(int argc, char *argv[])
{
    char mac[NET_MAC_LENGTH];

    if (2 != argc)
    {
        printf("Invalid arguments, need interface name as argument\n");
        return -1;
    }

    if (get_mac(argv[1], mac, sizeof(mac)))
    {
        printf("get mac address of %s failed\n", argv[1]);
        return -1;
    }

    printf("MAC Address: %s\n", mac);

    return 0;
}
