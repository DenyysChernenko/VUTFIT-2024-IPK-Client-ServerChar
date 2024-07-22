#include "UDP.h"
#include "errorproc.h"
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




// Messages Code
#define CONFIRM 0x00
#define REPLY 0x01
#define AUTH 0x02
#define JOIN 0x03
#define MSG 0x04
#define ERR 0xFE
#define BYE 0xFF


// Defines for Length
#define MAX_MSG_LENGTH 1400
#define MAX_USERNAME_LEN 20
#define MAX_SECRET_LEN 128
#define MAX_DISPLAYNAME_LEN 20
#define MAX_PENDING_MESSAGES 150 
#define MAX_USERNAME_LENGTH 20
#define MAX_DISPLAY_NAME_LENGTH 20
#define MAX_SECRET_LENGTH 128
#define MAX_MESSAGE_CONTENT_LENGTH 1400


// Mutexes/Handle Threads
pthread_cond_t auth_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_auth = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_msg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// Server Information Variables
int sockfd;
struct sockaddr_in server_addr;
uint16_t global_udp_timeout;
uint8_t global_max_retrans;
uint8_t retry_count = 0;
uint16_t current_retry_timeout = 0;



// Global Bool Variables
bool have_sent = false;
bool confirmation_received = true;
bool quit;
bool receive_thread_active = true;
bool AUTH_VER = false;
bool quit_requested = false;
bool retransimission_quit = false;
bool waiting_for_auth_reply = false;
bool result_auth_flag = false;


// Global Char Variables
char u_current_command[10];
char u_id[21]; 
char u_secret[129]; 
char u_dname[21];


// Function to generate message IDs correctly
uint16_t generate_message_id() {
    static uint16_t last_message_id = 0;
    return last_message_id++;
}


// Creating Queue to handle Sending Messages by User
typedef struct {
    char message[MAX_MSG_LENGTH];
    size_t length;
    uint16_t messageID;
    
} PendingMessage;


// Track Current Message ID to correct Send Messages
uint16_t current_MessageID =  0;


// Create Queue for Sending Messages
PendingMessage pending_messages[MAX_PENDING_MESSAGES];
size_t pending_messages_count = 0;


// Global Threads
pthread_t receive_thread, command_thread, send_thread;



/**
 * @brief Custom fgets function to read a line from a stream and remove newline characters.
 * 
 * This function reads a line from the specified stream and stores it into the buffer pointed to by str.
 * It removes any trailing newline characters ('\n' or '\r') from the input, if present.
 * 
 * @param str Pointer to a buffer where the line read is stored.
 * @param size Maximum number of characters to be read (including the null terminator).
 * @param stream Pointer to the FILE object that identifies the stream.
 * @return On success, the function returns str. If the end-of-file is encountered or an error occurs, NULL is returned.
 */
char *my_fgets(char *str, int size, FILE *stream) {
    if (fgets(str, size, stream) != NULL) {
        size_t len = strlen(str);
        if (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
            str[len - 1] = '\0';  
            if (len > 1 && str[len - 2] == '\r') {
                str[len - 2] = '\0';  
            }
        }
        return str;
    }
    return NULL;
}


/**
 * @brief Signal handler for SIGINT (Ctrl+C), SIGTERM, and SIGQUIT signals.
 * 
 * This function sends a BYE message to the server upon receiving a termination signal
 * (Ctrl+C or termination command). It sets the quit_requested flag to true, indicating
 * that the program should gracefully exit.
 */

void sigint_handler() {
    PendingMessage bye_message;
    bye_message.message[0] = 0xFF;
    uint16_t message_id = generate_message_id();
    bye_message.message[1] = (message_id >> 8) & 0xFF; 
    bye_message.message[2] = message_id & 0xFF;        
    bye_message.length = 3;
    bye_message.messageID = message_id;
    ssize_t sent_bytes = sendto(sockfd, bye_message.message, bye_message.length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        perror("sendto failed for BYE message");
    } 
    quit_requested = true;
}

/**
 * @brief Signal handler for end-of-file (EOF) signal.
 * 
 * This function sets the quit_requested flag to true upon detecting an end-of-file condition,
 * indicating that the program should gracefully exit.
 */

void eof_handler() {
    quit_requested = true;
}

/**
 * @brief Installs signal handlers for handling termination signals and end-of-file.
 * 
 * This function registers signal handlers for SIGINT, SIGTERM, and SIGQUIT signals to handle
 * termination requests, and it registers a handler for EOF to handle the end-of-file condition.
 * Additionally, it ignores the SIGTSTP signal (Ctrl+Z) to prevent stopping the program.
 */
void install_signal_handlers() {
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);  
    signal(SIGQUIT, sigint_handler);  
    signal(SIGTSTP, SIG_IGN);         
}




/**
 * @brief Thread function to receive messages from the server.
 * 
 * This function runs in a separate thread to continuously receive messages from the server.
 * It locks a mutex before processing the message to ensure.
 * thread safety and releases the mutex afterwards.
 * 
 * @return NULL
 */
void *receive_messages() {
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (receive_thread_active) {
        uint8_t response[MAX_MSG_LENGTH];
        ssize_t bytes_received = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr*)&from_addr, &from_len);
        if (bytes_received == -1) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        process_server_response(response, (size_t)bytes_received, ntohs(from_addr.sin_port));

        pthread_mutex_unlock(&mutex); 
    }

    return NULL;
}


/**
 * @brief Processes the response received from the server.
 * 
 * This function processes the response received from the server based on its message type.
 * Depending on the message type, it performs various actions such as handling confirmation,
 * CONFIRM,REPLY,MSG,ERR,BYE. For each message type, specific processing logic is implemented
 * including sending confirmation messages, error messages, or quitting the program if necessary.
 * 
 * @param response Pointer to the buffer containing the received response.
 * @param response_len Length of the received response.
 * @param src_port Source port from which the response was received.
 */

void process_server_response(uint8_t *response, size_t response_len, int src_port) {

    uint8_t message_type = response[0];
    uint16_t ref_messageID = ntohs(*(uint16_t *)(response + 1)); 


    switch (message_type) {
    case CONFIRM: {
            for (size_t i = 0; i < pending_messages_count; i++) {
                uint16_t current_MessageID = ntohs(*(uint16_t *)(pending_messages[i].message + 1));
                if (current_MessageID == ref_messageID) { 
                    for (size_t j = i; j < pending_messages_count - 1; ++j) {
                        memcpy(&pending_messages[j], &pending_messages[j + 1], sizeof(PendingMessage));
                    }
                    pending_messages_count--;

                    confirmation_received = true;
                    break; 
                }
            }
            break;
        }
    case REPLY: {
            uint8_t result = response[3];
            char messageContents[MAX_MSG_LENGTH];
            size_t msgContentLen = response_len - 6; 
            memcpy(messageContents, response + 6, msgContentLen);
            messageContents[msgContentLen] = '\0'; 

            if (result == 1) {
                if (strncmp(u_current_command, "/auth", 5) == 0) {
      
                    server_addr.sin_port = htons(src_port);                    
                    AUTH_VER = true;
                    pthread_cond_signal(&auth_cond);
                }
                fprintf(stderr, "Success: %s\n", messageContents);

            } else {
                if (strncmp(u_current_command, "/auth", 5) == 0) {
                    server_addr.sin_port = htons(src_port);   
                    result_auth_flag = true;
                    pthread_cond_signal(&auth_cond);

                }
                fprintf(stderr, "Failure: %s\n", messageContents);
            }
            send_confirmation_message(ntohs(*(uint16_t *)(response + 1)));
            break;
        }
    case ERR: {

        char display_name[21];
        strncpy(display_name, (char *)(response + 3), 20);
        display_name[20] = '\0';


        char message_content[MAX_MSG_LENGTH];
        size_t offset = 3 + strlen(display_name) + 1; 
        size_t message_content_len = strnlen((char *)(response + offset), response_len - offset);
        strncpy(message_content, (char *)(response + offset), message_content_len);
        message_content[message_content_len] = '\0';


        fprintf(stderr, "ERR FROM %s: %s\n", display_name, message_content);

        send_confirmation_message(ref_messageID);


        PendingMessage bye_message;
        bye_message.message[0] = 0xFF;
        uint16_t bye_message_id = generate_message_id();
        bye_message.message[1] = (bye_message_id >> 8) & 0xFF;
        bye_message.message[2] = bye_message_id & 0xFF;
        bye_message.length = 3;
        bye_message.messageID = bye_message_id;

        ssize_t sent_bytes = sendto(sockfd, bye_message.message, bye_message.length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("sendto failed for BYE message");
        } 
        quit = true;
        break;
    }

   case MSG: {
        size_t min_expected_length = 3;
        if (response_len < min_expected_length) {
            fprintf(stderr, "ERR: Message length is too short to contain valid data.\n");
            send_confirmation_message(ref_messageID);
            send_error_message(u_dname, "Message length is too short to contain valid data.");
            quit = true;
            return;
        }

        char display_name[MAX_DISPLAY_NAME_LENGTH + 1];
        strncpy(display_name, (char *)(response + 3), MAX_DISPLAY_NAME_LENGTH);
        display_name[MAX_DISPLAY_NAME_LENGTH] = '\0';
        size_t display_name_len = strnlen(display_name, MAX_DISPLAY_NAME_LENGTH);

        if (display_name_len == MAX_DISPLAY_NAME_LENGTH && display_name[MAX_DISPLAY_NAME_LENGTH] != '\0') {
            fprintf(stderr, "ERR: Display name is too long or not null-terminated.\n");
            send_confirmation_message(ref_messageID);
            send_error_message(u_dname, "Display name is too long or not null-terminated.");
            quit = true;
            return;
        }


        for (size_t i = 0; i < display_name_len; ++i) {
            if (!isprint(display_name[i]) || display_name[i] == '\n') {
                fprintf(stderr, "ERR: Invalid character in display name.\n");
                send_confirmation_message(ref_messageID);
                send_error_message(u_dname, "Invalid character in display name.");
                quit = true;
                return; 
            }
        }

        size_t content_offset = 3 + display_name_len + 1;
        size_t content_length = response_len - content_offset;

        if (content_length > 1400) { 
            fprintf(stderr, "ERR: Message content length exceeds maximum allowed length of 1400 characters.\n");
            send_confirmation_message(ref_messageID);
            send_error_message(u_dname, "Message content length exceeds maximum allowed length.");
            quit = true;
            return;
        }

        char message_content[1401]; 
        strncpy(message_content, (char *)(response + content_offset), content_length);
        message_content[content_length] = '\0'; 

        
        for (size_t i = 0; i < content_length; ++i) {
            if (message_content[i] < 0x20 || message_content[i] > 0x7E) {
                if(message_content[i] == 0x00) {
                    continue;
                } else {
                fprintf(stderr, "ERR: Invalid character in message content.\n");
                send_confirmation_message(ref_messageID);
                send_error_message(u_dname, "Invalid character in message content.");
                quit = true;
                return; 
                }
            }
        }

        printf("%s: %s\n", display_name, message_content);
        fflush(stdout);

        send_confirmation_message(ref_messageID);
        break;
    }

     case BYE: {
        send_confirmation_message(ref_messageID);

        uint8_t bye_message[3];
        bye_message[0] = BYE; 
        uint16_t bye_message_id = generate_message_id(); 
        *(uint16_t *)(bye_message + 1) = htons(bye_message_id); 

        ssize_t sent_bytes = sendto(sockfd, bye_message, sizeof(bye_message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("sendto failed for BYE message");
        }

  
        quit = true;
        break;
    }

   default:
    fprintf(stderr, "ERR: Received unknown message type: %hhu\n", message_type);
    

    char err_message[MAX_MSG_LENGTH];
    int err_msg_len = 0;
    err_message[err_msg_len++] = ERR;  
    

    uint16_t err_message_id = generate_message_id(); 
    err_message[err_msg_len++] = (err_message_id >> 8) & 0xFF;  
    err_message[err_msg_len++] = err_message_id & 0xFF;       


    strcpy(err_message + err_msg_len, u_dname);
    err_msg_len += strlen(u_dname) + 1;  


    const char* errMsgContents = "Unexpected message type received";
    strcpy(err_message + err_msg_len, errMsgContents);
    err_msg_len += strlen(errMsgContents) + 1; 

   
    ssize_t err_sent_bytes = sendto(sockfd, err_message, err_msg_len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err_sent_bytes < 0) {
        perror("sendto failed for ERR message");
    }


    char bye_message[3];
    bye_message[0] = BYE; 
    uint16_t bye_message_id = generate_message_id();
    bye_message[1] = (bye_message_id >> 8) & 0xFF;
    bye_message[2] = bye_message_id & 0xFF;


    ssize_t bye_sent_bytes = sendto(sockfd, bye_message, sizeof(bye_message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bye_sent_bytes < 0) {
        perror("sendto failed for BYE message");
    }
    
    quit = true;
    break;
    }

}

/**
 * @brief Processes user commands and sends messages.
 * 
 * This function continuously reads user input from stdin, processes the input commands,
 * and sends messages to the server. It handles various scenarios such as authentication,
 * sending messages, and quitting the program. Additionally, it waits for authentication
 * verification and message confirmation before proceeding. Upon receiving a quit request,
 * it sends a BYE message to the server and exits gracefully, cleaning up resources.
 */
void* process_commands() {
    char input[1500];

    while (!quit && !quit_requested) {
        memset(input, 0, sizeof(input));

        if (my_fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) {
                quit_requested = true;
                break;
            } else {
                perror("Error reading from stdin");
                break;
            }
        }

        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0') {
            continue; 
        }

        if (AUTH_VER == false && (!((strncmp(input, "/auth", 5) == 0) && (input[5] == ' ' || input[5] == '\0')))) {
            fprintf(stderr, "ERR: You need to verify first\n");
            continue;
        } else {
            if (input[0] == '/') {
                if(AUTH_VER == true && ((strncmp(input, "/auth", 5) == 0) && (input[5] == ' ' || input[5] == '\0'))){
                    fprintf(stderr, "ERR: You cannot auth twice\n");
                    continue;
                }       
                if ((strncmp(input, "/auth", 5) == 0) && (input[5] == ' ' || input[5] == '\0'))  {
                    u_handle_command(input);
                    pthread_mutex_lock(&mutex_auth);
                    while (!AUTH_VER && !result_auth_flag) {
                            pthread_cond_wait(&auth_cond, &mutex_auth);
                        }
                    pthread_mutex_unlock(&mutex_auth);
                    result_auth_flag = false;
                } else {                    
                    u_handle_command(input);
                }
                if (quit) break;
            } else {
                u_send_message(input);
            }


            pthread_mutex_lock(&mutex_msg);
            while (!have_sent) {
                pthread_cond_wait(&msg_cond, &mutex_msg);
            }
            pthread_mutex_unlock(&mutex_msg);
            have_sent = false;
        }
    }

    if (quit_requested) {
        PendingMessage bye_message;
        bye_message.message[0] = 0xFF;
        uint16_t message_id = generate_message_id();
        bye_message.message[1] = (message_id >> 8) & 0xFF; 
        bye_message.message[2] = message_id & 0xFF;        
        bye_message.length = 3;
        bye_message.messageID = message_id;
        ssize_t sent_bytes = sendto(sockfd, bye_message.message, bye_message.length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            perror("sendto failed for BYE message");
        } 

        pthread_mutex_lock(&mutex);
        pending_messages[pending_messages_count++] = bye_message;
        pthread_mutex_unlock(&mutex);
    }

    receive_thread_active = false;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&auth_cond);
    pthread_mutex_lock(&mutex); 
    pthread_mutex_unlock(&mutex);
    pthread_cancel(receive_thread);
    pthread_cancel(send_thread);
    exit(EXIT_SUCCESS); 
}



/**
 * @brief Connects to the server using UDP protocol.
 * 
 * This function establishes a UDP socket connection to the server specified by the provided
 * server port and address. It sets up signal handlers for handling interrupt signals, initializes
 * server address structure, and configures global parameters such as maximum retransmissions
 * and UDP timeout. It then creates three threads for receiving messages, processing commands,
 * and sending messages. The function waits for these threads to complete their tasks before
 * closing the socket connection and returning.
 * 
 * @param server_port The port number of the server.
 * @param server_address The IP address or hostname of the server.
 * @param udp_timeout The timeout value for UDP operations.
 * @param max_retransmissions The maximum number of retransmissions for failed messages.
 * @return Returns 0 on success, or -1 if an error occurs.
 */

int connect_to_server_udp(int server_port, char *server_address, uint16_t udp_timeout, uint8_t max_retransmissions) {
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

    install_signal_handlers(); 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    Inet_pton(AF_INET, server_address, &server_addr.sin_addr);
    global_max_retrans = max_retransmissions;
    global_udp_timeout = udp_timeout;


    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        perror("pthread_create for receive_messages");
        return -1;
    }

    if (pthread_create(&command_thread, NULL, process_commands, NULL) != 0) {
        perror("pthread_create for process_commands");
        return -1;
    }

    if (pthread_create(&send_thread, NULL, send_message_thread, NULL) != 0) {
        perror("pthread_create for send_message_thread");
        return -1;
    }

    pthread_join(command_thread, NULL);
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sockfd); 
    return 0; 
}

/**
 * @brief Handles user commands received from the command line.
 * 
 * This function parses the user input command and processes it accordingly. It supports
 * various commands such as authentication, joining channels, renaming display name, and
 * displaying help. It checks the format of the command and triggers the appropriate actions
 * or sends error messages if the command format is invalid or unknown.
 * 
 * @param command The user input command string.
 */
void u_handle_command(char *command) {
    if (strncmp(command, "/auth", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        strcpy(u_current_command, "/auth");
        char username[20], secret[128], displayName[20];
        if (sscanf(command, "/auth %49s %49s %49[^\n]", username, secret, displayName) == 3) {
            u_send_auth_message(command);
        } else {
            printf("Invalid /auth command format.\n");
            have_sent = true;
            pthread_cond_signal(&msg_cond);
            return;
        }
    } else if (strncmp(command, "/join", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        char channelID[50];
        strcpy(u_current_command, "/join");
        if (sscanf(command, "/join %49s", channelID) == 1) {
            u_send_join_message(channelID);
        } else {
            printf("Invalid /join command format.\n");
            have_sent = true;
            pthread_cond_signal(&msg_cond);
            return;
        }
    } else if (strncmp(command, "/rename", 7) == 0 && (command[7] == ' ' || command[7] == '\0')) {
        char displayName[MAX_USERNAME_LEN + 1]; 
        strcpy(u_current_command, "/rename");
        if (sscanf(command, "/rename %[^\n]", displayName) == 1) {

            if (strlen(displayName) > MAX_USERNAME_LEN) {
                fprintf(stderr, "ERR: Display name exceeds maximum allowed length\n");
                have_sent = true;
                pthread_cond_signal(&msg_cond);
                return;
            } else {
                strncpy(u_dname, displayName, sizeof(u_dname) - 1);
                u_dname[sizeof(u_dname) - 1] = '\0';
                have_sent = true;
                pthread_cond_signal(&msg_cond);
                return;
            }
        } else {
            have_sent = true;
            pthread_cond_signal(&msg_cond);
            fprintf(stderr, "ERR: Invalid format for rename command\n");
            return;
        }
    } else if (strncmp(command, "/help", 5) == 0 && (command[5] == ' ' || command[5] == '\0')) {
        strcpy(u_current_command, "/help");
        u_print_helps();
        have_sent = true;
        pthread_cond_signal(&msg_cond);
        return;
    } else {
        fprintf(stderr, "ERR: Unknown command. Type /help for a list of commands.\n");
        have_sent = true;
        pthread_cond_signal(&msg_cond);
        return;
    }
}


/**
 * @brief Sends an authentication message to the server.
 * 
 * This function extracts the username, secret, and display name from the provided command,
 * constructs an authentication message, and adds it to the pending messages queue. It checks
 * the lengths of the fields to ensure they do not exceed their maximum allowed lengths. If any
 * field exceeds its limit, an error message is displayed, and the program quits.
 * 
 * @param command The user input command containing authentication information.
 */
void u_send_auth_message(char* command) {
    const char delimiters[] = " \n"; 
    char* saveptr; 

    strtok_r(command, delimiters, &saveptr); 

    char* username = strtok_r(NULL, delimiters, &saveptr);
    char* secret = strtok_r(NULL, delimiters, &saveptr);
    char* displayName = strtok_r(NULL, delimiters, &saveptr);

    if (strlen(username) > MAX_USERNAME_LENGTH || strlen(displayName) > MAX_DISPLAY_NAME_LENGTH || strlen(secret) > MAX_SECRET_LENGTH) {
        fprintf(stderr, "ERR: One or more fields exceed their maximum allowed length.\n");
        quit = true;
        return;
    }

    char message[MAX_MSG_LENGTH];
    int msg_len = 0;

    uint8_t msgType = AUTH;
    message[msg_len++] = msgType;

    uint16_t message_id = generate_message_id(); 
    message[msg_len++] = (message_id >> 8) & 0xFF; 
    message[msg_len++] = message_id & 0xFF; 

    strcpy(message + msg_len, username);
    msg_len += strlen(username) + 1;

    strcpy(message + msg_len, displayName);
    msg_len += strlen(displayName) + 1; 

    strcpy(message + msg_len, secret);
    msg_len += strlen(secret) + 1; 

    PendingMessage new_message;
    memcpy(new_message.message, message, msg_len);
    new_message.length = msg_len;
    new_message.messageID = message_id;
    pending_messages[pending_messages_count++] = new_message;

    strncpy(u_id, username, sizeof(u_id) - 1); 
    u_id[sizeof(u_id) - 1] = '\0';
    strncpy(u_secret, secret, sizeof(u_secret) - 1);
    u_secret[sizeof(u_secret) - 1] = '\0';
    strncpy(u_dname, displayName, sizeof(u_dname) - 1); 
    u_dname[sizeof(u_dname) - 1] = '\0';
}


/**
 * @brief Sends a join message to the server.
 * 
 * This function constructs a join message containing the provided channel ID and the user's display name.
 * It then adds the message to the pending messages queue.
 * 
 * @param channelID The ID of the channel to join.
 */
void u_send_join_message(char* channelID) {
    char message[MAX_MSG_LENGTH];
    int msg_len = 0;

    uint8_t msgType = JOIN;
    message[msg_len++] = msgType;


    uint16_t message_id = generate_message_id(); 
    message[msg_len++] = (message_id >> 8) & 0xFF; 
    message[msg_len++] = message_id & 0xFF; 


    strcpy(message + msg_len, channelID);
    msg_len += strlen(channelID) + 1; 

  
    strcpy(message + msg_len, u_dname);
    msg_len += strlen(u_dname) + 1; 


    PendingMessage new_message;
    memcpy(new_message.message, message, msg_len);
    new_message.length = msg_len;
    new_message.messageID = message_id;
    pending_messages[pending_messages_count++] = new_message;


}

/**
 * @brief Sends a message to the server.
 * 
 * This function validates the length of the message content to ensure it does not exceed the maximum allowed length.
 * If the content is valid, it constructs a message containing the user's display name and the message content.
 * It then adds the message to the pending messages queue for further processing.
 * 
 * @param messageContent The content of the message to be sent.
 */

void u_send_message(char* messageContent) {
    if (strlen(messageContent) > MAX_MESSAGE_CONTENT_LENGTH) {
        fprintf(stderr, "ERR: Message content exceeds the maximum allowed length of %d characters.\n", MAX_MESSAGE_CONTENT_LENGTH);
        quit = true;
        return; 
    }

    printf("%s: %s\n", u_dname, messageContent);

    char message[MAX_MSG_LENGTH];
    int msg_len = 0;

    uint8_t msgType = MSG;
    message[msg_len++] = msgType;


    uint16_t message_id = generate_message_id(); 
    message[msg_len++] = (message_id >> 8) & 0xFF; 
    message[msg_len++] = message_id & 0xFF; 


    strcpy(message + msg_len, u_dname);
    msg_len += strlen(u_dname) + 1; 

    strcpy(message + msg_len, messageContent);
    msg_len += strlen(messageContent) + 1; 




    PendingMessage new_message;
    memcpy(new_message.message, message, msg_len);
    new_message.length = msg_len;
    new_message.messageID = message_id;
    pending_messages[pending_messages_count++] = new_message;

}

/**
 * @brief Sends messages to the server in a separate thread.
 * 
 * This function continuously checks for pending messages to be sent to the server.
 * If there are pending messages, it sends the first message in the queue to the server.
 * It then waits for a confirmation within a specified timeout period.
 * If a confirmation is received, it signals that the message has been sent successfully.
 * If no confirmation is received within the timeout, it retries sending the message for a maximum number of times.
 * If the maximum number of retransmissions is reached without confirmation, it quits the application.
 * 
 * @return NULL
 */

void *send_message_thread() {
    int retransmission_count = 0; 
    while (!quit  && !retransimission_quit && !quit_requested) {

        if (pending_messages_count > 0) {
            PendingMessage message = pending_messages[0];
            ssize_t sent_bytes = sendto(sockfd, message.message, message.length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            if (sent_bytes < 0) {
                perror("sendto failed");

                return NULL;
            }

            
            current_MessageID = message.messageID;
            confirmation_received = false;


            usleep(global_udp_timeout * 1000);


            if (confirmation_received) {
                have_sent = true;
                pthread_cond_signal(&msg_cond);
                retransmission_count = 0;
            } else {
                retransmission_count++;
                if (retransmission_count >= global_max_retrans) {
                    
                    have_sent = true;
                    pthread_cond_signal(&msg_cond);
                    quit = true;
                    receive_thread_active = false;


                    PendingMessage bye_message;
                    bye_message.message[0] = 0xFF;
                    uint16_t bye_message_id = generate_message_id();
                    bye_message.message[1] = (bye_message_id >> 8) & 0xFF;
                    bye_message.message[2] = bye_message_id & 0xFF;
                    bye_message.length = 3;
                    bye_message.messageID = bye_message_id;
                    ssize_t bye_sent_bytes = sendto(sockfd, bye_message.message, bye_message.length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                    if (bye_sent_bytes < 0) {
                        perror("sendto failed for BYE message");
                    } 


                    pthread_mutex_lock(&mutex);
                    pending_messages[pending_messages_count++] = bye_message;
                    pthread_mutex_unlock(&mutex);

                    break;
                }
            }


        } else {

            usleep(1000);
        }
    }


    pthread_cancel(command_thread);
    pthread_cancel(receive_thread);
    pthread_cancel(send_thread);
    close(sockfd);
    exit(EXIT_SUCCESS);
}



/**
 * @brief Sends a confirmation message to the server for a received message.
 * 
 * This function constructs a confirmation message and sends it to the server 
 * to confirm the receipt of a message identified by its message ID.
 * 
 * @param messageID The ID of the message to confirm.
 */

void send_confirmation_message(uint16_t messageID) {

    uint8_t confirm_message[3];
    confirm_message[0] = CONFIRM;
    confirm_message[1] = (messageID >> 8) & 0xFF;
    confirm_message[2] = messageID & 0xFF;

    ssize_t sent_bytes = sendto(sockfd, confirm_message, sizeof(confirm_message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        perror("sendto failed for CONFIRM message");
    }
}


/**
 * @brief Sends an error message to the server.
 * 
 * This function constructs an error message containing the display name and message contents 
 * and sends it to the server to report an error condition.
 * 
 * @param display_name The display name associated with the error.
 * @param message_contents The contents of the error message.
 */

void send_error_message(const char* display_name, const char* message_contents) {
    size_t display_name_len = strlen(display_name);
    size_t message_contents_len = strlen(message_contents);

    if (display_name_len > MAX_DISPLAY_NAME_LENGTH || message_contents_len > MAX_MESSAGE_CONTENT_LENGTH) {
        fprintf(stderr, "ERR: Display name or message content length exceeds maximum allowed length.\n");
        return;
    }


    uint16_t messageID = generate_message_id();


    uint8_t err_message[MAX_MSG_LENGTH];
    int msg_len = 0;

    uint8_t msgType = ERR;
    err_message[msg_len++] = msgType;

    err_message[msg_len++] = (messageID >> 8) & 0xFF;
    err_message[msg_len++] = messageID & 0xFF;

    strcpy((char *)(err_message + msg_len), display_name);
    msg_len += display_name_len + 1; 
    strcpy((char *)(err_message + msg_len), message_contents);
    msg_len += message_contents_len + 1; 

    ssize_t sent_bytes = sendto(sockfd, err_message, msg_len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent_bytes < 0) {
        perror("sendto failed for ERR message");
    } 
}


/**
 * @brief Prints the list of supported local commands.
 * 
 * This function prints the list of supported local commands to the console.
 */
void u_print_helps() {
    printf("Supported local commands:\n");
    printf("/auth {ID} {Secret} {DisplayName} - Authenticate with the server\n");
    printf("/join {ChannelID} - Join a channel\n");
    printf("/rename {DisplayName} - Change your display name\n");
    printf("/help - Display this help message\n");
}