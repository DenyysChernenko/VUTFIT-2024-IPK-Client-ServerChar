#include "errorproc.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <netdb.h>
#include <string.h>


int Socket(int domain, int type, int protocol) {
    int res = socket(domain,type,protocol);

    if (res < 0) {
        perror("ERR: Socket Failed\n");
        exit(EXIT_FAILURE);
    }
    return res;
}


void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int res = connect(sockfd, addr, addrlen);

    if(res == -1) {
        perror("ERR: Connection failed");
        exit(EXIT_FAILURE);
    }

}


void Inet_pton(int af, const char *source, void *destination) {
    struct addrinfo hints, *res;
    int status;

    // Set hints to filter results
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af; // AF_INET or AF_INET6
    hints.ai_socktype = SOCK_STREAM; // Stream socket

    // Resolve the hostname to IP address
    if ((status = getaddrinfo(source, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Copy the resolved IP address to the destination
    if (af == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        memcpy(destination, &ipv4->sin_addr, sizeof(struct in_addr));
    } else if (af == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
        memcpy(destination, &ipv6->sin6_addr, sizeof(struct in6_addr));
    }

    freeaddrinfo(res); // Free memory allocated by getaddrinfo

    // You can now use the resolved IP address
}