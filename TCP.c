#include "TCP.h"
#include "errorproc.h"
#include "supportTCP.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>


// All Mutexes/Handle Threads
pthread_cond_t t_auth_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t t_mutex_auth = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t t_mutex_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t_msg_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t control = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t t_rename_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t t_rename_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t_join_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t join = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t dname_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t server_thread;

// Bool Global Variables
bool rename_cond = false;
bool join_cond = false;
bool t_result_auth_flag = false;
bool t_msg_condition = false;
bool t_reply = false;
bool AUTH_FLAG = false;


// Char Global Variables
char id[21]; 
char secret[129]; 
char dname[21];
char current_command[10];

// Socket Global Function
int t_sockfd;

// Signal Holder , For CTRL+C
void t_sigint_holder() {
    send_bye_message();
    _Exit(EXIT_SUCCESS); 
}


/**
 * @brief Establishes a TCP connection to the server and handles user input.
 * 
 * This function creates a TCP socket, connects to the server, and starts a listener thread.
 * It then enters a loop to continuously read user input from the console.
 * Depending on the input, it handles commands such as authentication, joining, renaming, or sending messages.
 * 
 * @param server_port The port number of the server.
 * @param server_address The IP address or hostname of the server.
 * @return Returns 0 upon successful execution.
 */
int connect_to_server_tcp(int server_port, char* server_address) {
    t_sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    Inet_pton(AF_INET, server_address, &server_addr.sin_addr);
    Connect(t_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));


    pthread_create(&server_thread, NULL, listen_to_server, (void*)&t_sockfd);
    while (1) {
        char input[1500];
        
        signal(SIGINT, t_sigint_holder);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) {
                send_bye_message();
                break;
            } else {
                perror("fgets");
                send_bye_message();
                exit(EXIT_FAILURE);
            }
        }
        input[strcspn(input, "\n")] = '\0';

        if (!(strncmp(input, "/auth", 5) == 0 && (input[5] == ' ' || input[5] == '\0')) && AUTH_FLAG == false)
                {
            fprintf(stderr, "ERR: You need to authenticate first\n");
        } else {
            if (input[0] == '/') {
                if(AUTH_FLAG == true && ((strncmp(input, "/auth", 5) == 0) && (input[5] == ' ' || input[5] == '\0'))){
                    fprintf(stderr, "ERR: You cannot auth twice\n");
                    continue;
                }    
                 if ((strncmp(input, "/auth", 5) == 0) && (input[5] == ' ' || input[5] == '\0')) {
                    handle_command(input);
                    pthread_mutex_lock(&t_mutex_auth);
                    while (!AUTH_FLAG && !t_result_auth_flag) {
                            pthread_cond_wait(&t_auth_cond, &t_mutex_auth);
                        }
                    pthread_mutex_unlock(&t_mutex_auth);
                    t_result_auth_flag = false;


                 }

                else if((strncmp(input, "/join", 5) == 0) && (input[5] == ' ' || input[5] == '\0')) {
                    handle_command(input);
                    pthread_mutex_lock(&join);
                    while (!join_cond && !t_reply) {
                            pthread_cond_wait(&t_join_cond, &join);
                        }
                    pthread_mutex_unlock(&join);
                    join_cond = false;
                    t_reply = false;

              }  else if((strncmp(input, "/rename", 7) == 0) && (input[7] == ' ' || input[7] == '\0')) {
                    handle_command(input);
                    pthread_mutex_lock(&t_rename_mutex);
                    while (!rename_cond) {
                            pthread_cond_wait(&t_rename_cond, &t_rename_mutex);
                        }
                    pthread_mutex_unlock(&t_rename_mutex);
                    rename_cond = false;
                    t_msg_condition = true;
                    pthread_cond_signal(&t_msg_cond);
                } else {
                     handle_command(input);
                     }
            } else {
                send_message(input, dname);
            }

            pthread_mutex_lock(&t_mutex_msg);
            while (!t_msg_condition) {
                    pthread_cond_wait(&t_msg_cond, &t_mutex_msg);
                }
            pthread_mutex_unlock(&t_mutex_msg);
            t_msg_condition = false;

        }
    }
    return 0;
}


/**
 * @brief Listener thread function to receive messages from the server.
 * 
 * This function continuously listens for incoming messages from the server.
 * When a message is received, it calls the handle_server_message function to process it.
 * If the server closes the connection or an error occurs during reception, it prints an error message.
 * 
 * @param arg A pointer to the file descriptor of the socket connected to the server.
 * @return Always returns NULL.
 */
void* listen_to_server(void* arg) {
    int t_sockfd = *(int*)arg;
    char message[1500];
    int bytes_received;
    
    while (1) {
    bytes_received = recv(t_sockfd, message, sizeof(message), 0);
    
    if (bytes_received > 0) {
        handle_server_message(message);
    } else if (bytes_received == 0) {
        fprintf(stderr, "ERR: Server closed the connection.\n");
        break;
    } else {
        if (errno == EINTR) {
            continue;
        }
        perror("recv");
        exit(EXIT_FAILURE);
        }
    }
      return NULL;
}




/**
 * @brief Handles the incoming command from the client.
 * 
 * This function examines the received command and determines its type.
 * Based on the type of command, it calls the corresponding handler function.
 * 
 * @param command The received command string.
 * 
 */
void handle_command(char* command) {
    if (strncmp(command, "/auth", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        strcpy(current_command, "/auth");
        handle_auth_command(command);
    } else if (strncmp(command, "/join", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        strcpy(current_command, "/join");
        handle_join_command(command);
    } else if (strncmp(command, "/rename", 7) == 0 && (command[7] == ' ' || command[7] == '\0')) {
        strcpy(current_command, "/rename");
        handle_rename_command(command);
    } else if (strncmp(command, "/help", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        strcpy(current_command, "/help");
        handle_help_command();
    } else {
        strcpy(current_command, "Unknown");
        fprintf(stderr, "ERR: Unknown command\n");
        t_msg_condition = true;
        pthread_cond_signal(&t_msg_cond);
        return;

    }
}

/**
 * @brief Handles the server messages received from the socket.
 * 
 * This function processes the server messages according to their types.
 * For 'REPLY' messages, it handles 'OK' and 'NOK' responses.
 * For 'MSG' messages, it validates and prints the message content.
 * For 'ERR' messages, it checks the format and prints the error message.
 * It also sends appropriate error messages back to the server and closes the connection if necessary.
 * 
 * @param message The received message from the server.
 * @param t_sockfd The file descriptor of the socket connected to the server.
 */
void handle_server_message(char* message) {
    if (message == NULL) {
        fprintf(stderr, "ERR: Null message received\n");
        return;
    }

    if (strncasecmp(message, current_command, strlen(current_command)) == 0) {
        return;
    }

    char trimmed_message[1500];
    sscanf(message, " %1499[^\n] ", trimmed_message);

    char* token = strtok(trimmed_message, " ");
    // Convert token to lowercase
    for (int i = 0; token[i]; i++) {
        token[i] = tolower(token[i]);
    }

    if (strcasecmp(token, "REPLY") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            if (strcasecmp(token, "OK") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL && strcasecmp(token, "IS") == 0) {
                    token = strtok(NULL, ""); 
                    if (token != NULL) {
                        fprintf(stderr, "Success: %s\n", token);
                        if (strcasecmp(current_command, "/auth") == 0) {
                            AUTH_FLAG = true;
                            pthread_cond_signal(&t_auth_cond);
                        }
                        if (strcasecmp(current_command, "/join") == 0) {
                            join_cond = true;
                            pthread_cond_signal(&t_join_cond);
                        }
                    } else {
                        fprintf(stderr, "ERR: No response after 'IS'\n");
                    }
                } else {
                    fprintf(stderr, "ERR: Unexpected response after 'OK'\n");
                }
            } else if (strcasecmp(token, "NOK") == 0) {
                token = strtok(NULL, " ");
                if (token != NULL && strcasecmp(token, "IS") == 0) {

                    token = strtok(NULL, "");
                }
                if (token != NULL) {
                    if (strcasecmp(current_command, "/auth") == 0) {
                        t_result_auth_flag = true;
                        pthread_cond_signal(&t_auth_cond);
                    }
                    if (strcasecmp(current_command, "/join") == 0) {
                        t_reply = true;
                        pthread_cond_signal(&t_join_cond);
                    }
                    fprintf(stderr, "Failure: %s\n", token);
                } else {
                    fprintf(stderr, "ERR: No response after 'NOK'\n");
                }
            } else {
                fprintf(stderr, "ERR: Unknown response after 'REPLY'\n");
            }
        } else {
            fprintf(stderr, "ERR: No response after 'REPLY'\n");
        }
    }  else if (strcasecmp(token, "MSG") == 0) {
        char* from_keyword = strtok(NULL, " "); 
        char* sender = strtok(NULL, " "); 
        char* is_keyword = strtok(NULL, " "); 
        char* message_content = strtok(NULL, ""); 


        if (from_keyword == NULL || strcasecmp(from_keyword, "FROM") != 0 ||
            sender == NULL || is_keyword == NULL || strcasecmp(is_keyword, "IS") != 0 ||
            message_content == NULL || !is_valid_dname(sender) || !is_valid_content(message_content)) {

            fprintf(stderr, "ERR: Malformed 'MSG' command received.\n");
            send_error_and_close_connection(t_sockfd, "Invalid MSG format.");
            return; 
        }

        printf("%s: %s\n", sender, message_content);
        fflush(stdout);
    }   else if (strcasecmp(token, "ERR") == 0) {
        char* fromKeyword = strtok(NULL, " "); 
        char* displayName = strtok(NULL, " "); 
        char* isKeyword = strtok(NULL, " "); 
        char* errorMessage = strtok(NULL, "");

        if (fromKeyword != NULL && strcasecmp(fromKeyword, "FROM") == 0 &&
            displayName != NULL && isKeyword != NULL && strcasecmp(isKeyword, "IS") == 0 &&
            errorMessage != NULL) {
            fprintf(stderr, "ERR FROM %s: %s\n", displayName, errorMessage);
        } else {
            fprintf(stderr, "ERR: Received malformed ERR message from server.\n");
            char errorMsg[255];
            if (strlen(dname) > 0) {
                snprintf(errorMsg, sizeof(errorMsg), "ERR FROM %s IS Malformed ERR message.\r\n", dname);
            } else {
                snprintf(errorMsg, sizeof(errorMsg), "ERR FROM IS Malformed ERR message.\r\n");
            }
            send(t_sockfd, errorMsg, strlen(errorMsg), 0);
            send_bye_message();

            shutdown(t_sockfd, SHUT_RDWR); 
            close(t_sockfd);
            exit(EXIT_FAILURE);
        }

        send_bye_message();
        shutdown(t_sockfd, SHUT_RDWR); 
        close(t_sockfd);
        exit(EXIT_FAILURE);
    } else if (strcasecmp(token, "BYE") == 0) {
        send_bye_message(); 
        
        shutdown(t_sockfd, SHUT_RDWR); 
        close(t_sockfd); 
        
        exit(EXIT_SUCCESS);
    } else {
        fprintf(stderr, "ERR: Unknown message format received.\n");
        char errorMsg[255];
        if (strlen(dname) > 0) {
            snprintf(errorMsg, sizeof(errorMsg), "ERR FROM %s IS Invalid message format.\r\n", dname);
        } else {
            snprintf(errorMsg, sizeof(errorMsg), "ERR FROM C IS Invalid message format.\r\n");
        }
        send(t_sockfd, errorMsg, strlen(errorMsg), 0);

        send_bye_message();
        exit(EXIT_FAILURE); 
    }
}




/**
 * @brief Handles the "/auth" command received from the user.
 * 
 * This function parses the command and extracts the username, secret, and display name.
 * If the parameters are valid, it sends an authentication message to the server.
 * Otherwise, it prints an error message and signals the message condition.
 * 
 * @param command The "/auth" command with all information received from the user.
 * 
 */
void handle_auth_command(char* command) {
    int count = sscanf(command, "/auth %s %s %s", id, secret, dname);
    if (count == 3) {
        send_auth_message(id, secret, dname);
    } else {
        fprintf(stderr, "ERR: Invalid parameters for /auth command\n");
        t_msg_condition = true;
        pthread_cond_signal(&t_msg_cond);
        return;
    }
}


/**
 * @brief Handles the "/rename" command received from the user.
 * 
 * This function extracts the new display name from the command and updates the current display name.
 * If the parameters are valid, it signals the rename condition.
 * Otherwise, it prints an error message and signals the rename condition.
 * 
 * @param command The "/rename" with displayname command received from the user.
 */
void handle_rename_command(char* command) {
    char new_display_name[50]; 
    int count = sscanf(command, "/rename %50[^\n]", new_display_name);
    if (count == 1) {
        strncpy(dname, new_display_name, sizeof(dname) - 1);
        dname[sizeof(dname) - 1] = '\0'; 
        rename_cond = true;
        pthread_cond_signal(&t_rename_cond);
    } else {
        fprintf(stderr, "ERR: Invalid parameters for /rename command\n");
        rename_cond = true;
        pthread_cond_signal(&t_rename_cond);
    }
}

/**
 * @brief Handles the "/join" command received from the user.
 * 
 * This function extracts the channel ID from the command and sends a join message to the server.
 * If the parameters are valid, it sends the join message.
 * Otherwise, it prints an error message and signals the message condition.
 * 
 * @param command The "/join" with channel command received from the user.
 * 
 */
void handle_join_command(char* command) {
    char channel_id[256];
    int count = sscanf(command, "/join %20[A-Za-z0-9]", channel_id);
    if (count == 1) {
        send_join_message(channel_id, dname);
    } else {
        fprintf(stderr, "ERR: Invalid parameters for /join command\n");
        t_msg_condition = true;
        pthread_cond_signal(&t_msg_cond);
        return;
    }
}

/**
 * @brief Displays the help message for supported local commands.
 * 
 * This function prints out the list of supported local commands along with their descriptions.
 * It also signals the message condition to indicate completion.
 */

void handle_help_command() {
    printf("Supported local commands:\n");
    printf("/auth {ID} {Secret} {DisplayName} - Authenticate with the server\n");
    printf("/join {ChannelID} - Join a channel\n");
    printf("/rename {DisplayName} - Change your display name\n");
    printf("/help - Display this help message\n");
    t_msg_condition = true;
    pthread_cond_signal(&t_msg_cond);
}


/**
 * @brief Sends an authentication message to the server.
 * 
 * This function constructs and sends an authentication message to the server
 * using the provided username(id), secret, and display name. It checks if the provided
 * username, secret, and display name adhere to the grammar rules before sending the message.
 * If the input is valid, the message is sent to the server, and the message condition
 * is signaled to indicate completion. Otherwise, an error message is printed, and the
 * message condition is still signaled.
 * 
 * @param id The username for authentication.
 * @param secret The secret for authentication.
 * @param dname The display name for authentication.
 */

void send_auth_message(char* id, char* secret, char* dname) {

    if(is_valid_secret(secret) && is_valid_dname(dname) && is_valid_id(id)) {
            char auth_message[1500];
            sprintf(auth_message, "AUTH %s AS %s USING %s\r\n", id, dname, secret);
            send(t_sockfd, auth_message, strlen(auth_message), 0);
            t_msg_condition = true;
            pthread_cond_signal(&t_msg_cond);
    } else {
        fprintf(stderr, "ERR: Inccorect according to Grammar\n");
        t_msg_condition = true;
        pthread_cond_signal(&t_msg_cond);
    }
    
}

/**
 * @brief Sends a join message to the server.
 * 
 * This function constructs and sends a join message to the server
 * using the provided channel ID and display name. It then signals
 * the message condition to indicate completion.
 * 
 * @param channel_id The ID of the channel to join.
 * @param display_name The display name of the user joining the channel.
 */
void send_join_message(char* channel_id, char* display_name) {
    char join_message[1500];
    sprintf(join_message, "JOIN %s AS %s\r\n", channel_id, display_name);
    send(t_sockfd, join_message, strlen(join_message), 0);
    t_msg_condition = true;
    pthread_cond_signal(&t_msg_cond);
}

/**
 * @brief Sends a message to the server.
 * 
 * This function constructs and sends a message to the server
 * using the provided message content and display name. It then
 * signals the message condition to indicate completion.
 * 
 * @param message The content of the message to send.
 * @param display_name The display name of the user sending the message.
 */

void send_message(char* message, const char* display_name) {

    char display_name_local[21]; 
    strncpy(display_name_local, display_name, sizeof(display_name) - 1);
    display_name_local[sizeof(display_name) - 1] = '\0'; 
    

    if(is_valid_dname(display_name_local)) {
    char generic_message[1500];
    snprintf(generic_message, sizeof(generic_message), "MSG FROM %s IS %s\r\n", display_name_local, message);
    printf("%s: %s\n", display_name_local, message); 
    send(t_sockfd, generic_message, strlen(generic_message), 0);
    t_msg_condition = true;
    pthread_cond_signal(&t_msg_cond);
    } else {
        fprintf(stderr, "ERR: Wrong Char in Rename\n");
        t_msg_condition = true;
        pthread_cond_signal(&t_msg_cond);
    }
}


/**
 * @brief Sends a "BYE" message to the server to indicate disconnection.
 * 
 * This function sends a "BYE" message to the server to indicate
 * that the client is disconnecting. It sends the message using the
 * socket file descriptor associated with the connection.
 */
void send_bye_message() {
    send(t_sockfd, "BYE\r\n", 5, 0);
}


/**
 * @brief Sends an error message to the server and closes the connection.
 * 
 * This function sends an error message to the server along with a "BYE"
 * message to indicate disconnection. It closes the connection associated
 * with the provided socket file descriptor.
 * 
 * @param t_sockfd The socket file descriptor of the connection.
 * @param error_message The error message to send to the server.
 */
void send_error_and_close_connection(int t_sockfd, const char* error_message) {


    if (dname != NULL && strlen(dname) > 0) {
        char err_msg_with_dname[300];
        snprintf(err_msg_with_dname, sizeof(err_msg_with_dname), "ERR FROM %s IS %s\r\n", dname, error_message);
        send(t_sockfd, err_msg_with_dname, strlen(err_msg_with_dname), 0);
    } else {
        char err_msg[255];
        snprintf(err_msg, sizeof(err_msg), "ERR FROM Client IS %s\r\n", error_message);
        send(t_sockfd, err_msg, strlen(err_msg), 0);
    }

    char bye_msg[] = "BYE\r\n";
    send(t_sockfd, bye_msg, strlen(bye_msg), 0);

    shutdown(t_sockfd, SHUT_RDWR);
    close(t_sockfd);


    extern pthread_t server_thread;
    if (server_thread != 0) {
        pthread_join(server_thread, NULL);
    }

    exit(EXIT_FAILURE); 
}