#include "errorproc.h"
#include "TCP.h"
#include "UDP.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


// Function prototypes
void print_help();
void process_arguments(int argc, char *argv[]);


// Global variables for storing argument values
char transport_protocol[4];
char server_address[256];   
uint16_t server_port = 4567;
uint16_t udp_timeout = 250;
uint8_t max_retransmissions = 3;

int main(int argc, char *argv[]) {
    process_arguments(argc, argv);
    
    int sockfd;
    if (strcmp(transport_protocol, "tcp") == 0) {
        sockfd = connect_to_server_tcp(server_port, server_address);
    } else if (strcmp(transport_protocol, "udp") == 0) {
        sockfd = connect_to_server_udp(server_port, server_address, udp_timeout, max_retransmissions);
    } else {
        fprintf(stderr, "ERR: Invalid transport protocol specified.\n");
        exit(EXIT_FAILURE);
    }
    close(sockfd);

    return 0;
}

/**
 * @brief Displays the help message for Program usage.
 * 
 * This function prints out the list of supported arguments along with their descriptions.
 * 
 */
void print_help() {
    printf("Program usage:\n");
    printf("-t <transport_protocol> (tcp or udp)\n");
    printf("-s <server_address> (IP address or hostname)\n");
    printf("-p <server_port> (default: 4567)\n");
    printf("-d <udp_timeout> (default: 250)\n");
    printf("-r <max_retransmissions> (default: 3)\n");
    printf("-h (prints program help and exits)\n");
}



/**
 * @brief Processes command line arguments passed to the program.
 * 
 * This function iterates through the command line arguments and extracts relevant information 
 * such as transport protocol, server address, port number, UDP timeout, and maximum retransmissions.
 * If the "-h" option is provided, it displays the program's help message and exits. 
 * If mandatory arguments (-t and -s) are missing, it prints an error message and exits.
 * 
 * @param argc The number of command line arguments.
 * @param argv An array of strings containing the command line arguments.
 */
void process_arguments(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            strcpy(transport_protocol, argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0) {
            strcpy(server_address, argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0) {
            server_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0) {
            udp_timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0) {
            max_retransmissions = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help();
            exit(0);
        }
    }

    if (strcmp(transport_protocol, "") == 0 || strcmp(server_address, "") == 0) {
        fprintf(stderr, "ERR: Mandatory arguments -t and -s are missing.\n");
        print_help();
        exit(1);
    }
}


