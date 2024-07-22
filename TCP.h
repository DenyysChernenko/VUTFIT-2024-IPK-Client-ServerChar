#ifndef TCP_H
#define TCP_H

#include <ctype.h>
#include <stdbool.h>
#include <string.h>








int connect_to_server_tcp(int server_port, char* server_address);
void handle_command(char* command);


void handle_auth_command(char* command);
void send_auth_message(char* username, char* secret, char* display_name);
void handle_join_command(char* command);
void send_join_message(char* channel_id, char* display_name);
void handle_rename_command(char* command);
void handle_help_command();
void send_message(char* input, const char* display_name);
void send_bye_message();


void* listen_to_server(void* arg);
void handle_server_message(char* message);
void send_error_and_close_connection(int sockfd, const char* error_message);


#endif