#ifndef ERRORPROC_H
#define ERRORPROC_H


#include <sys/types.h>
#include <sys/socket.h>


int Socket(int domain, int type, int protocol);
void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void Inet_pton(int af, const char *source, void *destionation);


#endif