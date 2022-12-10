#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>

#include "ipc.h"

#define PORT 5000
struct sockaddr_in serv_addr;
int client_fd;

int create_socket()
{
    int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
	return sock;
}

int connect_socket(int fd)
{
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address\n");
        return -1;
    }
 
    if ((client_fd = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
	return client_fd;
}

ssize_t send_socket(int fd, const char *buf, size_t len)
{
	send(fd, buf, len, 0);
	return -1;
}

ssize_t recv_socket(int fd, char *buf, size_t len)
{
	read(fd, buf, len);
	return -1;
}

void close_socket(int fd)
{
	close(client_fd);
}
