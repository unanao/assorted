/**
 * @file check-net.c
 * @brief Check if the interface is connected
 * @author Jianjiao Sun <jianjiaosun@163.com>
 * @version 1.0
 * @date 2015-06-15
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>  
#include <string.h>

/**
 * @brief Check if the  specified interface is connected
 *
 * @param nic_name Interface name
 *
 * @return 	0 	Connected
 			!0	Disconnected
 */
int check_nic(char *nic_name)
{
	struct ifreq ifr;
	int skfd;
	int ret = -1;

	if ((skfd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, nic_name, sizeof(ifr.ifr_name));
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) >=  0)
	{
		if(ifr.ifr_flags & IFF_RUNNING)
		{
			ret = 0; 
		}
	}

	close(skfd);

	return ret;
}

int main()
{
	if (!check_nic("eth0"))
	{
		printf("connected\n");
	}
	else
	{
		printf("disconnected\n");
	}

	return 0;
}
