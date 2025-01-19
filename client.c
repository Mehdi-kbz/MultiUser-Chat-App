#include <arpa/inet.h>    // For converting IP addresses
#include <netdb.h>        // For getaddrinfo and addrinfo structure
#include <netinet/in.h>   // For sockaddr_in and other network-related definitions
#include <stdio.h>        // Standard input/output functions
#include <stdlib.h>       // Standard library for memory management, exit functions
#include <string.h>       // For handling strings (e.g., memset, strlen, etc.)
#include <sys/socket.h>   // For socket programming (socket, connect, send, recv)
#include <unistd.h>       // For file descriptor manipulation (close)
#include <poll.h>         // For polling I/O events (poll, struct pollfd)
#include <ctype.h>
#include "common.h"       // Custom common functions (possibly defined elsewhere)
#include "msg_struct.h"
#include <sys/ioctl.h> 

// Initialization of variables
ClientInfo *clientList = NULL;
Channel *channel_list = NULL;

// Function to check nickname
int check_nickname(char *nickname) {
    // check nickname length
    if (!nickname || strlen(nickname) < 1 || strlen(nickname) >= NICK_LEN) {
        printf( "[Server]:"  " The nickname length must be between 1 and %d characters.\n" , NICK_LEN - 1);
        return 0;
    }

    // check if nickname is reserved
    if (strcasecmp(nickname, "Server") == 0) {
        printf( "[Server]:"  " The nickname 'Server' is reserved. Please choose a different one.\n" );
        return 0;
    }

    // Check if the nickname already exists and is valid
    for (ClientInfo *current = clientList; current; current = current->next) {
        if (strcasecmp(nickname, current->nickname) == 0) {
            printf( "[Server]:"  " This nickname is already in use. Please choose a different one.\n" );
            return 0;
        }
    }

    // check if nickname is alphanumerical
    for (size_t i = 0; i < strlen(nickname); i++) {
        if (!isalnum(nickname[i])) {
            printf( "[Server]:"  " Invalid nickname. Only letters and numbers are allowed.\n" );
            return 0;
        }
    }
    return 1;
}

// Function to check channel name validity
int check_channel_name(char *channel) {
    // Check if channel length is within the allowed range
    if (strlen(channel) < 1 || strlen(channel) >= CHAN_LEN) {
        printf( "[Server]:"  " Channel name must be between 1 and %d characters.\n" , CHAN_LEN - 1);
        return 0;
    }

    // Ensure channel name contains only alphanumeric characters
    for (char *ch = channel; *ch != '\0'; ch++) {
        if (!isalnum(*ch)) {
            printf( "[Server]:"  " Channel name must contain only letters and numbers.\n" );
            return 0;
        }
    }

    // Confirm the channel name is not already in use
    for (ClientInfo *client = clientList; client != NULL; client = client->next) {
        if (strcmp(channel, client->channel) == 0) {
            printf( "[Server]:"  " Channel name already taken. Please select a different name.\n" );
            return 0;
        }
    }

    return 1;
}

// Function to welcome the clients
void send_greetings(struct currentClientInfo *client) {
printf( "\n[Server] :"" Welcome to the chat" " %s"" !\n\n",client->nickname);
printf("You can use the following commands:\n\n"  "'/nick'"  " : to change your nickname\n"
                    "'/who'"  " : to display a list of all the connected users.\n"
                    "'/whois + <username>'"  " : to display information about a user named username.\n"
                    "'/whoami'"  " : to display information about you.\n"
                    "'/msg + <username> + message'"  " : to send a private message to a user called username.\n"
                    "'/msgall + message'"  " : to send a broadcast message.\n"
                    "'/create + <channel_name>'"  " : to create a channel named channel_name.\n"
                    "'/channel_list'"  " : to display all the available channels.\n"
                    "'/join + <channel_name>'"  " : to join a channel called channel_name.\n"
                    "'/send + <receiver_name> + <file_name>'"  " : to send a file to the user receiver_name.\n"
                    "'/quit'"  " : to quit. ( quits a channel if the user is in a channel or the server if not ).\n\n"
                   "If no command is used, an echo message or channel message will be sent.\n\n> ");}

// Function to add a user
struct currentClientInfo* add_user(int sockfd, struct sockaddr_in address) {
    
    struct currentClientInfo* new = malloc(sizeof(struct currentClientInfo));
    if (!new) {
        perror("malloc");
        return NULL;
    }

    // Set socket and address information
    new->sockfd = sockfd;
    new->address = address;

    // Initializing fields
    memset(new->nickname, 0, NICK_LEN);
    new->connect_time = time(NULL);
    inet_ntop(AF_INET, &address.sin_addr, new->ip_address, INET_ADDRSTRLEN);
    new->port_number = ntohs(address.sin_port);
    memset(new->channel, 0, CHAN_LEN);
    return new;
}

// Function to find a client using nickname
ClientInfo* nick_to_client(ClientInfo *list, char *nick) {
    ClientInfo *curr = list;

    while (curr != NULL) {
        if (strcmp(curr->nickname, nick) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL; 
}


////////////////////////// File Functions  //////////////////////////
void handle_file_reception(struct currentClientInfo *currentClient, char *sender_nick, char *file_name) {
    char buffer_pld[MSG_LEN];

    printf("[Server]:"" %s"" wants to send you the file named '%s'.\n  Do you accept? [Y/N]: ", sender_nick, file_name);
    char response = getchar();
    getchar(); 

    struct message msg;

    if (response == 'Y' || response == 'y') {
        int sockfd = 0;
        int new_sock = 0;
        struct sockaddr_in server_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                perror("socket");
                exit(1);
        }


        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8181);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("bind");
                exit(1);
        }


        if (listen(sockfd, SOMAXCONN) == 0) {
            printf("[Server]:"" Receiving the file...\n");
        } else {
                perror("Error: could not listen");
                exit(1);
        }

        msg.type = FILE_ACCEPT;
        strncpy(msg.infos, sender_nick, INFOS_LEN - 1);
        msg.infos[INFOS_LEN - 1] = '\0';

        strncpy(buffer_pld, file_name, MSG_LEN - 1);
        buffer_pld[MSG_LEN - 1] = '\0';

        msg.pld_len = strlen(buffer_pld);


        if (send(currentClient->sockfd, &msg, sizeof(struct message), 0) <= 0) {
                perror("send");
            printf("[Server]:" " Error sending FILE_ACCEPT struct to the sender.\n");
            close(sockfd);
            return;
        }

        if (send(currentClient->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
            perror("send");
                close(sockfd);
            return;
        }

        struct sockaddr_in new_addr;
        socklen_t addr_size = sizeof(new_addr);
        new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);
        if (new_sock < 0) {
                perror("accept");
                close(sockfd);
            exit(1);
        }

        char buffer_pld_ack[MSG_LEN];
        struct message msg_recv;

        if (recv(new_sock, &msg_recv, sizeof(struct message), 0) <= 0) {
                perror("recv");
                close(new_sock);
            close(sockfd);
            return;
        }

        write_in_new_file(new_sock, file_name, currentClient, buffer_pld_ack, msg_recv);

        char currentDir[1024]; // Buffer to hold the current directory

        // Get the current working directory
        if (getcwd(currentDir, sizeof(currentDir)) != NULL) {
            printf("[Server]:" " File saved in %s/file.txt.\n", currentDir);
        } else {
            perror("getcwd() error");
        }        
        


        close(new_sock);
        close(sockfd);

    } else if (response == 'N' || response == 'n') {
        // Send FILE_REJECT message to the sender
        msg.type = FILE_REJECT;
        strncpy(msg.infos, sender_nick, INFOS_LEN - 1);
        msg.infos[INFOS_LEN - 1] = '\0';

        strncpy(buffer_pld, "File reception rejected.", MSG_LEN);
        msg.pld_len = strlen(buffer_pld);

        if (send(currentClient->sockfd, &msg, sizeof(struct message), 0) <= 0) {
            perror("send");
            return;
        }

        if (send(currentClient->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
            perror("send");
            return;
        }

        
    } else {
     printf("\nOops :(\n");
    }
}

void send_file(char *server_name, char *server_port, char *file_path, int socket_file) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MSG_LEN];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8181);  // Use the same port for file transfer
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    

    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("fopen");
        printf("Error: could not open the file.\n ""> ");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    struct message send_msg;
    send_msg.type = FILE_SEND;

    char file_name[FILENAME_LEN];
    strncpy(file_name, file_path, FILENAME_LEN - 1);
    file_name[FILENAME_LEN - 1] = '\0';
    send_msg.pld_len = file_size;

    if (send(sockfd, &send_msg, sizeof(struct message), 0) <= 0) {
        perror("send");
        fclose(file);
        close(sockfd);
        exit(1);
    }
        printf( "[Server]:"" Client accepted file transfert.\n");

    size_t total_bytes_sent = 0;
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sockfd, buffer, bytes_read, 0) <= 0) {
            perror("send");
            fclose(file);
            close(sockfd);
            exit(1);
        }

        total_bytes_sent += bytes_read;

        fflush(stdout);
    }

    // Envoyer la taille du fichier après les données réelles
    if (send(sockfd, &total_bytes_sent, sizeof(size_t), 0) <= 0) {
        perror("send");
        fclose(file);
        close(sockfd);
        exit(1);
    }

    fclose(file);


    printf("[Server]:"" Connecting to client and sending the file...\n");
    const int width = 30;  // Width of the animation area
    const char *fileIcon = "\033[1;34m📄\033[0m"; // Blue file icon
    const int totalProgress = 101; // Total percentage for completion
    int progress = 0; // Start at 0%

    for (int i = 0; i <= totalProgress; ++i) {
        int position = i % (width * 2);
        position = position < width ? position : (width * 2 - position); // Bounce effect

        printf("\r"); // Return to the beginning of the line

        // Print spaces to position the file icon
        for (int j = 0; j < position; j++) putchar(' ');

        // Print the file icon and the progress percentage
        printf("%s %3d%%", fileIcon, progress);
        
        // Clear the remaining part of the line to avoid leftover characters
        printf("\033[K");

        fflush(stdout);

        usleep(50000); // 50ms delay
        progress = i; // Update progress percentage
    }

    printf("\n");
    struct message msg_ack;

    char buffer_pld_ack[MSG_LEN];


    if (recv(sockfd, &msg_ack, sizeof(struct message), 0) <= 0) {
        perror("recv");
        return;
    }


    if (recv(sockfd, buffer_pld_ack, msg_ack.pld_len, 0) <= 0) {
        perror("recv");
        return;
    }
    
    printf("[Server]:"" Client has received the file.\n");
   
    close(sockfd);
}

void write_in_new_file(int sockfd, char *file_name, struct currentClientInfo *currentClient, char *buff, struct message msg) {
    FILE *received_file = fopen("file.txt", "wb");

    if (received_file == NULL) {
        perror("fopen");
        close(sockfd);
        return;
    }
    
    const int width = 30;  // Width of the animation area
    const char *fileIcon = "\033[1;34m📄\033[0m"; // Blue file icon
    const int totalProgress = 101; // Total percentage for completion
    int progress = 0; // Start at 0%

    for (int i = 0; i <= totalProgress; ++i) {
        int position = i % (width * 2);
        position = position < width ? position : (width * 2 - position); // Bounce effect

        printf("\r"); // Return to the beginning of the line

        // Print spaces to position the file icon
        for (int j = 0; j < position; j++) putchar(' ');

        // Print the file icon and the progress percentage
        printf("%s %3d%%", fileIcon, progress);
        
        // Clear the remaining part of the line to avoid leftover characters
        printf("\033[K");

        fflush(stdout);

        usleep(50000); // 50ms delay
        progress = i; // Update progress percentage
    }

    printf("\n");





    size_t file_size = msg.pld_len;

    char buffer[MSG_LEN];

    size_t total_bytes_received = 0;
    while (total_bytes_received < file_size) {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("recv");
            fclose(received_file);
            close(sockfd);
            return;
        }

        size_t bytes_written = fwrite(buffer, 1, bytes_received, received_file);

        if (bytes_written != bytes_received) {
            perror("fwrite");
            fclose(received_file);
            close(sockfd);
            return;
        }

        total_bytes_received += bytes_received;

        fflush(stdout);
    }

    fclose(received_file);


    struct message ack_msg;
    char buffer_pld[MSG_LEN];

    strncpy(ack_msg.nick_sender, currentClient->nickname, NICK_LEN - 1);
    ack_msg.nick_sender[NICK_LEN - 1] = '\0';
    ack_msg.type = FILE_ACK;
    strncpy(ack_msg.infos, file_name, INFOS_LEN - 1);
    ack_msg.infos[INFOS_LEN - 1] = '\0';
    ack_msg.pld_len = snprintf(buffer_pld, MSG_LEN, "File received successfully");

    if (send(sockfd, &ack_msg, sizeof(struct message), 0) <= 0) {
        perror("send");
    }

    if (send(sockfd, buffer_pld, ack_msg.pld_len, 0) <= 0) {
        perror("send");
    }

    close(sockfd);
}




// Function to handle client's input
void handle_message(char *input, struct message *msgstruct, char *nick_sender, char *buffer_pld, size_t buffer_size, struct currentClientInfo *currentClient) {
    char *command;
    char args[MSG_LEN];
    strncpy(args, input, MSG_LEN);
    memset(msgstruct, 0, sizeof(struct message));
    msgstruct->type = UNKNOWN_COMMAND;
    strncpy(msgstruct->nick_sender, nick_sender, NICK_LEN - 1);
    msgstruct->nick_sender[NICK_LEN - 1] = '\0';

    if (strchr(args, ' ') != NULL) {
        command = strtok(args, " ");
    } 
    else {
        command = strtok(args, " \n");
    }
    if (command == NULL) {
        return;
    }


    // check if command starts with '/'
    if (command[0] == '/' && 
        strcmp(command, "/nick") != 0 && 
        strcmp(command, "/who") != 0 && 
        strcmp(command, "/whois") != 0 &&         
        strcmp(command, "/whoami") != 0 && 
        strcmp(command, "/msg") != 0 && 
        strcmp(command, "/msgall") != 0 &&
        strcmp(command, "/create") != 0 &&
        strcmp(command, "/channel_list") != 0 &&
        strcmp(command, "/join") != 0 &&
        strcmp(command, "/quit") != 0 &&
        strcmp(command, "/quit (to quit a channel") != 0 &&
        strcmp(command, "/channel_send") != 0 &&
        strcmp(command, "/send") != 0 ){
        return;
    }


    
    // handle /nick command
    if (strcmp(command, "/nick") == 0) {
        msgstruct->type = NICKNAME_NEW;

        char *n_nick = strtok(NULL, "\n");
        if (!n_nick) {
            printf("[Server]: "" Nickname missing. Use '/nick <new_nickname>'\n" );
            return;
        }
        n_nick[strcspn(n_nick, "\n")] = '\0';
        strncpy(msgstruct->infos, n_nick, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';

        // Update the nickname in currentClient
        strncpy(currentClient->nickname, n_nick, NICK_LEN - 1);
        currentClient->nickname[NICK_LEN - 1] = '\0';

    } 

    // handle /who command
    else if (strcmp(command, "/who") == 0) {
        msgstruct->type = NICKNAME_LIST;
    }

    // handle /whoami command
    else if (strcmp(command, "/whoami") == 0) {
        msgstruct->type = WHOAMI;

    }
     
    // handle /whois command
    else if (strcmp(command, "/whois") == 0) {
        msgstruct->type = NICKNAME_INFOS;
        char *arg = strtok(NULL, "\n");
        if (!arg) {
            printf("[Server]:"" Username missing. Use '/whois <username>'\n" );
            return;
        }
        arg[strcspn(arg, "\n")] = '\0';
        strncpy(msgstruct->infos, arg, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';


    } 
    
    // handle /msgall command
    else if (strcmp(command, "/msgall") == 0) {
        msgstruct->type = BROADCAST_SEND;
        char *payload_strt = strtok(NULL, "\n");
        if (!payload_strt) {
            printf( "[Server]:"" Message missing. Use '/msgall <message>'\n" );
            return;
        }
        msgstruct->pld_len = strlen(payload_strt);
        if (msgstruct->pld_len >= buffer_size) {
            printf( "[Server]: "" Message is too long.\n" );
            return;
        }

        strncpy(buffer_pld, payload_strt, buffer_size);



    } 
    
    // handle /msg command
    else if (strcmp(command, "/msg") == 0) {
        msgstruct->type = UNICAST_SEND;
        char *args = strtok(NULL, " ");
        if (!args) {
            printf( "[Server]:"" missing. Use '/msg <user> <message>'\n" );
            return;
        }
        char *payload_strt = strtok(NULL, "\n");
        if (!payload_strt) {
            printf( "[Server]:"" format incorrect. Use '/msg <user> <message>'\n" );
            return;
        }
        args[strcspn(args, " ")] = '\0';
        msgstruct->pld_len = strlen(payload_strt);
        if (msgstruct->pld_len >= buffer_size) {
            printf( "[Server]:"" Message is too long.\n" );
            return;
        }
        strncpy(msgstruct->infos, args, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';
        strncpy(buffer_pld, payload_strt, buffer_size);
    } 
    
    // handle /create command
    else if (strcmp(command, "/create") == 0) {
        msgstruct->type = MULTICAST_CREATE;
        char *c_name = strtok(NULL, "\n");
        if (!c_name){
            printf( "[Server]:"" Channel name missing. Use '/create <channel_name>'\n" );
            return;
        }
        if (check_channel_name(c_name) == 1){
            c_name[strcspn(c_name, "\n")] = '\0';
            strncpy(msgstruct->infos, c_name, INFOS_LEN - 1);
            msgstruct->infos[INFOS_LEN - 1] = '\0';
        }
        // Update the nickname in currentClient
        strncpy(currentClient->channel, c_name, CHAN_LEN - 1);
        currentClient->channel[CHAN_LEN - 1] = '\0';
    } 
    
    // handle /channel_list command
    else if (strcmp(command, "/channel_list") == 0) {
        msgstruct->type = MULTICAST_LIST;
    } 
    
    // handle /join command
    else if (strcmp(command, "/join") == 0) {
        msgstruct->type = MULTICAST_JOIN;
        char *c_joining = strtok(NULL, "\n");
        if (!c_joining) {
            printf( "[Server]:"" Channel name is missing. Use '/join <channel_name>'\n" );
            return;
        }
        c_joining[strcspn(c_joining, "\n")] = '\0';
        strncpy(msgstruct->infos, c_joining, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';
        // Update the nickname in currentClient
        strncpy(currentClient->channel, c_joining, CHAN_LEN - 1);
        currentClient->channel[CHAN_LEN - 1] = '\0';


    } 
    
    // handle echo / multicast send
    else if (command[0] != '/') {
        msgstruct->pld_len = strlen(input);
        if (strlen(currentClient->channel) == 0) {
            strncpy(msgstruct->infos, "", INFOS_LEN - 1);
        } else {
            strncpy(msgstruct->infos, currentClient->channel, INFOS_LEN - 1);
        }
        msgstruct->infos[INFOS_LEN - 1] = '\0';
        if (strlen(input) >= buffer_size) {
            printf( "[Server]:"" Message too long.\n" );
            return;
        }
        strncpy(buffer_pld, input, buffer_size);
        msgstruct->type = (strlen(currentClient->channel) == 0) ? ECHO_SEND : MULTICAST_SEND;
    } 
    
    // handle /send command
    else if (strcmp(command, "/send") == 0) {
        msgstruct->type = FILE_REQUEST;
        
        char *receiver = strtok(NULL, " ");
        if (!receiver) {
            printf( "[Server]:"" File receiver's name is missing. Use '/send <nickname_receiver> <file_name>'\n" );
            return;
        }

        char *f_name = strtok(NULL, "\n");
        if (!f_name) {
            printf( "[Server]: "" File name is missing. Use '/send <nickname_receiver> <file_name>'\n" );
            return;
        }

        size_t f_name_length = strlen(f_name);
        if (f_name_length >= MSG_LEN) {
            printf( "[Server]:""  File name too long. Increase buffer size.\n" );
            return;
        }

        strncpy(msgstruct->infos, receiver, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';
        strncpy(buffer_pld, f_name, f_name_length);
        buffer_pld[f_name_length] = '\0';
        msgstruct->pld_len = f_name_length;
    } 
    
    
    // handle /quit command
    else if (strcmp(command, "/quit") == 0) {
        msgstruct->type = QUIT_REQUEST;
        strncpy(msgstruct->infos, msgstruct->nick_sender, INFOS_LEN - 1);
        msgstruct->infos[INFOS_LEN - 1] = '\0';

        if (strlen(currentClient->channel) == 0) {
            // If the client is not in any channel, break
            msgstruct->type = SERVER_QUIT;
            return;
        } else {
            // If the client is in a channel, set type to MULTICAST_QUIT
            msgstruct->type = MULTICAST_QUIT;

            // Set the channel name in the infos field
            strncpy(msgstruct->infos, currentClient->channel, INFOS_LEN - 1);
            msgstruct->infos[INFOS_LEN - 1] = '\0';
            memset(currentClient->channel, '\0', CHAN_LEN);
        }
    } 
   
   // handle unknown commands
    else {
        msgstruct->type = UNKNOWN_COMMAND;
        strncpy(buffer_pld, input, buffer_size);
    }
}

// Function to display feedback on client's terminal
void echo_client(struct message msgstruct, char *nickname, char *buffer_pld) {
    switch (msgstruct.type) {
        case UNKNOWN_COMMAND:
            printf( "[Server]: "  "Unknown command.\n" );
            printf("You can use the following commands:\n\n"  "'/nick'"  " : to change your nickname\n"
                    "'/who'"  " : to display a list of all the connected users.\n"
                    "'/whois + <username>'"  " : to display information about a user named username.\n"
                    "'/whoami'"  " : to display information about you.\n"
                    "'/msg + <username> + message'"  " : to send a private message to a user called username.\n"
                    "'/msgall + message'"  " : to send a broadcast message.\n"
                    "'/create + <channel_name>'"  " : to create a channel named channel_name.\n"
                    "'/channel_list'"  " : to display all the available channels.\n"
                    "'/join + <channel_name>'"  " : to join a channel called channel_name.\n"
                    "'/send + <receiver_name> + <file_name>'"  " : to send a file to the user receiver_name.\n"
                    "'/quit'"  " : to quit. ( quits a channel if the user is in a channel or the server if not ).\n\n"
                   "If no command is used, an echo message or channel message will be sent.\n\n");
            printf("> ");
            break;
        
        case NICKNAME_NEW:
            printf( "[Server]:"  " Nickname set successfully: %s\n" , nickname);
            printf("> ");
            break;
        
        case NICKNAME_SUCCESS:
            printf( "[Server]:"  " Nickname changed successfully.\n" );
            printf("> ");
            break;
        
        case NICKNAME_ERROR:
            printf( "[Server]:"  " Error: could not change nickname. (invalid or already in use)\n" );
            printf("> ");
            break;
        
        case NICKNAME_LIST:
            printf( "[Server]:"  " %s\n", buffer_pld);
            printf("> ");
            break;
        
        case NICKNAME_INFOS:
            printf("%s", buffer_pld);
            printf("> ");
            break;
        
        case NICKNAME_INFOS_ERROR:
            printf( "[Server]:"  " Error: user not found. Use '/who' to check online users.\n" );
            printf("> ");
            break;
        
        case ECHO_SEND:
            printf( "[Server]:"  " Message echoed: %s " , buffer_pld);
            printf("> ");
            break;
        
        case BROADCAST_SUCCESS:
            printf( "[Server]:"  " Broadcast Message sent.\n" );
            printf("> ");
            break;
        
        case BROADCAST_ERROR:
            printf( "[Server]:"  " Error: could not send Broadcast Message.\n" );
            printf("> ");
            break;
        
        case BROADCAST_SEND:
            printf( "[%s]: "  "%s\n", msgstruct.nick_sender, buffer_pld);
            printf("> ");
            break;
        
        case UNICAST_SUCCESS:
            printf( "[Server]:"  " Unicast Message sent.\n" );
            printf("> ");
            break;
        
        case UNICAST_ERROR:
            printf( "[Server]:"  " Error: could not send Unicast Message. (Invalid format / user not found.)\n" );
            break;
        
        case UNICAST_SEND:
            printf( "[%s]: "  "%s\n", msgstruct.nick_sender, buffer_pld);
            printf("> ");
            break;
        
        case MULTICAST_CREATE_SUCCESS:
            printf( "\n[Server]:"  " You have created channel"  " %s.\n" , msgstruct.infos);
            printf( "\n----------------------------------\n"  "      You have joined "  "%s.\n", msgstruct.infos);
            printf("----------------------------------\n"  "> " );
            break;
        
        case MULTICAST_CREATE_ERROR:
            printf( "[Server]:"  " Channel creation unsuccessful. Invalid / Channel already exists?\n" );
            printf( "> ");
            break;
        
        case MULTICAST_JOIN_SUCCESS:
            printf( "\n----------------------------------\n"  "      You have joined "  "%s.\n", msgstruct.infos);
            printf("----------------------------------\n"  "> " );
            break;
        
        case MULTICAST_JOIN_ERROR:
            printf( "[Server]:"  " Failed to join channel.\n" );
            break;
        
        case MULTICAST_LIST:
            printf( "[Server]:"  "%s", buffer_pld);
            printf( "> ");
            break;
        
        case MULTICAST_SEND:
            printf("%s", buffer_pld);
            printf( "> " );
            break;
        
        case MULTICAST_SEND_SUCCESS:
            printf( "[Server]:"  " Multicast message sent.\n" );
            printf( "> " );
            break;
        
        case MULTICAST_SEND_ERROR:
            printf( "[Server]:"  " Error: sending Multicast message.\n" );
            printf( "> " );
            break;
        
        case MULTICAST_QUIT_SUCCESS:
            printf( "\n----------------------------------\n"  "      You have left "  "%s.\n", msgstruct.infos);
            printf("----------------------------------\n" );
            printf( "> ");
            break;
        
        case MULTICAST_QUIT_ERROR:
            printf( "[Server]:"  " Error: Failed to leave channel.\n" );
            printf( "> " );
            break;
        
        case FILE_REJECT:
            printf( "[Server]:"  " Client refused file transfer.\n" );
            printf("> ");
            break;
        
        case FILE_EXISTENCE_ERROR:
            printf( "[Server]:"   " File not found.\n" );
            printf("> ");
            break;

        case RECEIVER_EXISTENCE_ERROR:
            printf( "[Server]:"   " Receiver not found. Use ""'/who'"" to check online users.\n" );
            printf("> ");
            break;

        case TRY_AGAIN_Y_N:
            printf("[Server]:"" Invalid response.\n Use the letters 'y' or 'n' to accept/refuse the file transfer\n");
            printf("> ");
            break;

        default:
            printf("> ");
            break;
    }
}

// Function to handle client
void handle_client(int sockfd, struct currentClientInfo* currentClient) {
    char buff[MSG_LEN];
    char buff_nick[MSG_LEN];
    int nickname_set = 0;
    int sockfd_file = -1;
    char nick_sender[NICK_LEN];
    struct message msgstruct;
    char buffer_pld[MSG_LEN];
    struct pollfd fds[MAX_EVENTS];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    printf("\n");
    fflush(stdout);

    while (1) {
        int ret = poll(fds, MAX_EVENTS, -1);
        if (ret == -1) {
            perror("poll");
            break;
        }

        if (!nickname_set) {
            while (1) {
                memset(&msgstruct, 0, sizeof(struct message));
                memset(buff_nick, 0, MSG_LEN);

                if (fgets(buff_nick, MSG_LEN, stdin) == NULL) {
                    printf("[Server]:""  Error while entering the nickname.\n");
                    break;
                }
                buff_nick[strcspn(buff_nick, "\n")] = '\0';

                if (strncmp(buff_nick, "/nick", 5) != 0) {
                    printf("[Server]:"" use '/nick <nickname>' to set your nickname.\n");
                    printf("[Server]:"" Please enter your nickname:\n" );
                    continue;
                }

                if (strncmp(buff_nick, "/nick", 5) == 0) {
                    char *nick_arg = buff_nick + 6;
                    strncpy(nick_sender, nick_arg, NICK_LEN);

                    if (check_nickname(nick_sender) == 1) {
                        msgstruct.type = NICKNAME_NEW;
                        msgstruct.pld_len = strlen(nick_sender);
                        strncpy(msgstruct.nick_sender, nick_sender, NICK_LEN);
                        strncpy(msgstruct.infos, nick_sender, NICK_LEN);

                        if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                            perror("send");
                            break;
                        }
                        struct message server_message;
                        if (recv(sockfd, &server_message, sizeof(struct message), 0) <= 0) {
                            perror("recv");
                            break;
                        }

                        if (server_message.type == NICKNAME_SUCCESS) {
                            
                            strncpy(currentClient->nickname, nick_arg, NICK_LEN - 1);
                            currentClient->nickname[NICK_LEN - 1] = '\0';
                            printf("\n[Server]:"" Nickname set successfully.\n");
                            send_greetings(currentClient);
                            fflush(stdout);
                            nickname_set = 1;
                            break;
                        } 
                        else if (server_message.type == NICKNAME_ERROR) {
                            printf("[Server]: "" Nickname already in use. Choose a different one.\n\n");
                            printf("[Server]:"" Please enter your nickname:\n");
                            continue;
                        }
                    } 
                    else {
                        printf( "\n[Server]:"" Invalid command. Use '/nick <nickname>' to set your nickname.\n\n");
                        printf("[Server]:"" Please enter your nickname:\n\n");
                        continue;
                    }
                }
                printf("[Server]:"" Please enter your nickname:\n\n");
            }
        }
        memset(&msgstruct, 0, sizeof(struct message));

        if (fds[0].revents & (POLLIN | POLLRDNORM)) {
            memset(&msgstruct, 0, sizeof(struct message));
            memset(buffer_pld, 0, MSG_LEN);
            if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                perror("recv");
                break;
            }
            if (msgstruct.pld_len > 0) {
                if (recv(sockfd, buffer_pld, msgstruct.pld_len, 0) <= 0) {
                    perror("recv");
                    break;
                }
                buffer_pld[msgstruct.pld_len] = '\0';
                if (msgstruct.type == NICKNAME_SUCCESS) {
                    strncpy(nick_sender, msgstruct.infos, NICK_LEN - 1);
                    nick_sender[NICK_LEN - 1] = '\0';
                } 
                else if (msgstruct.type == FILE_REQUEST) {
                    handle_file_reception(currentClient, msgstruct.nick_sender, buffer_pld);
                }
                else if (msgstruct.type == FILE_ACCEPT) {        
                    send_file(CLIT_ADDR, CLIT_PORT, buffer_pld, sockfd_file);
                }
                
                else if (msgstruct.type == FILE_SEND) {    
                }
                else if (msgstruct.type == FILE_ACK) {    
                }

            }
            echo_client(msgstruct,msgstruct.nick_sender, buffer_pld);
        }

        fflush(stdout);
        memset(&msgstruct, 0, sizeof(struct message));
        memset(buffer_pld, 0, MSG_LEN);

        if (fds[1].revents & POLLIN) {
            memset(&msgstruct, 0, sizeof(struct message));
            memset(buff, 0, MSG_LEN);
            memset(buffer_pld, 0, MSG_LEN);
            if (fgets(buff, MSG_LEN, stdin) == NULL) {
                break;
            }
            handle_message(buff, &msgstruct, nick_sender, buffer_pld, MSG_LEN, currentClient);
            if (msgstruct.type == SERVER_QUIT) {
                break;
            }
            if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                perror("send");
                memset(&msgstruct, 0, sizeof(struct message));
                break;
            }

            if (msgstruct.pld_len > 0) {
                if (send(sockfd, buffer_pld, msgstruct.pld_len, 0) <= 0) {
                    perror("send");
                    memset(buffer_pld, 0, MSG_LEN);
                    break;
                }
            }
        }
        memset(&msgstruct, 0, sizeof(struct message));
        memset(buffer_pld, 0, MSG_LEN);
    }
    printf( "\n-------------------------------------------\nDisconnected from the server");
        fflush(stdout);
        for (int i = 0; i < 3; ++i){
            printf(".");
            fflush(stdout);
            usleep(200000);
        }
        printf(" Goodbye :)\n" );
        usleep(100000);
        fflush(stdout);
        printf( "-------------------------------------------\n");
}

// Function to establish a connection to the server
int handle_connect(const char *server_name, const char *server_port) {
    struct addrinfo hints, *result, *rp;  // addrinfo struct to store possible server addresses
    int sfd;  // Socket file descriptor

    memset(&hints, 0, sizeof(struct addrinfo));  // Clear the hints structure
    hints.ai_family = AF_UNSPEC;     // Allow either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP (stream sockets)

    // Get server address information
    if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
        perror( "getaddrinfo()" );  // Display error if address resolution fails
        exit(EXIT_FAILURE);  // Exit with failure code
    }

    // Iterate through possible server addresses and try to connect
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);  // Create a socket
        if (sfd == -1) {
            continue;  // Skip to next if socket creation fails
        }
        // Try to connect to the server
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;  // Stop once we successfully connect
        }
        close(sfd);  // Close the socket if connection fails
    }

    // If no connection was successful, exit with an error
    if (rp == NULL) {
        fprintf(stderr, "Could not connect.\n" );
        exit(EXIT_FAILURE);
    }
 	else
    {
        printf( "\n----------------------------------");
        printf("\nConnecting to server");
        fflush(stdout);
        for (int i = 0; i < 3; ++i){

            printf(".");
            fflush(stdout);
            usleep(200000);
        }
        printf(" done!\n" );
        usleep(100000);
        fflush(stdout);
        printf( "----------------------------------");
        printf("\n\n""[server] :"" login with /nick <your username> \n");
	}
    freeaddrinfo(result);  // Free the address info linked list
    return sfd;  // Return the socket file descriptor for the established connection
}
 
// Main Function
int main(int argc, char *argv[]) {
    
    // Ensure the user provides exactly 2 arguments (server name and port)
    if (argc != 3) {
        fprintf(stderr, "Missing arguments. Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *server_name = argv[1];
    const char *server_port = argv[2];
    int sfd = handle_connect(server_name, server_port);
    struct sockaddr_in server_address;
    socklen_t addrlen = sizeof(struct sockaddr_in);
   
    if (getpeername(sfd, (struct sockaddr*)&server_address, &addrlen) == -1) {
        perror( "getpeername()" );
        close(sfd);
        exit(EXIT_FAILURE);
    }

    struct currentClientInfo* currentClient = add_user(sfd, server_address);
    handle_client(sfd, currentClient);
    // remove user
    if (currentClient) {
        free(currentClient);
    }
    close(sfd);
    return EXIT_SUCCESS;
}
