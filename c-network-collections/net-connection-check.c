/**
 * @file client.c
 * @brief Check if can connected to the specified address
 * @author Jianjiao Sun <jianjiaosun@163.com>
 * @version 1.0
 * @date 2016-02-17
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#define CONNECT_TIME_OUT 10

static check_options(int sock)
{
    int ret = -1;
    int error;
    int len = sizeof(error);

    if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        perror("get socket option failed.\n");
        printf("errno = %d.\n", errno);
        ret =  -1;
    }
    else 
    {
        if(error != 0)
        {
            printf("connect to server failed, error = %d.\n", error);
        }
        else
        {
            printf("connected to server success.\n");

            printf("errno = %d.\n", errno);

            ret = 0;
        }
    }
}

int check_connect(char *port, char *address)
{
    int socketfd;
    struct sockaddr_in server_addrinfo;
    fd_set wtset;
    struct timeval tm = {CONNECT_TIME_OUT,0};
    int error = -1;
    socklen_t length = sizeof(int);
    int flags;
    int ret;
    int err;

    memset(&server_addrinfo, 0, sizeof(server_addrinfo));
    server_addrinfo.sin_family = AF_INET;
    server_addrinfo.sin_port = htons(atoi(port));
    if(inet_pton(AF_INET, address, &server_addrinfo.sin_addr) < 0)
    {
        printf("server addr failed.\n");
        return -1;
    }

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0)
    {
        printf("server addr failed.\n");
        return -1;
    }

    flags = fcntl(socketfd,F_GETFL);
    fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);

    printf("connecting to server...\n");
    if(connect(socketfd, (struct sockaddr*)&server_addrinfo, sizeof(server_addrinfo)) < 0)
    {
        FD_ZERO(&wtset);
        FD_SET(socketfd,&wtset);
        err = select(socketfd + 1,NULL, &wtset, NULL, &tm);
        if(err == -1)
        {
            perror("select socket failed.\n");
            ret = -1;
        }
        else if(err == 0)
        {
            printf("connected to server timeout.\n");
            ret = -1;
        }
        else
        {
            ret = check_options(socketfd);
        }
    }
    close(socketfd);
    return ret;
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Please input valid arguments.\n");
        return -1;
    }

    return check_connect(argv[1], argv[2]);
}
