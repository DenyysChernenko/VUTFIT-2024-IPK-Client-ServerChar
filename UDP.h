#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <pthread.h> 
#include <signal.h>

#ifndef UDP_H
#define UDP_H


// Main functions
int connect_to_server_udp(int server_port, char *server_address, uint16_t udp_timeout, uint8_t max_retransmissions);
void u_handle_command(char *command);
void* receive_messages();
void* process_commands();
void *send_message_thread();

// Messages
void u_send_auth_message(char* command);
void u_send_join_message(char* channelID);
void u_send_message(char* messageContent);
void u_print_helps();
void send_error_message(const char* display_name, const char* message_contents);
void send_confirmation_message(uint16_t messageID);

//Additional functions
uint16_t generate_message_id();
void process_server_response(uint8_t *response, size_t response_len, int src_port);
void send_message_to_server(uint8_t* message, size_t msg_len);
void process_error_message(uint8_t *error_message);


#endif