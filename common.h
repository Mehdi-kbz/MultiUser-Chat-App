#ifndef COMMON_H
#define COMMON_H

#include <time.h>            // Include time library for tracking connection time
#include "msg_struct.h"      // Include msgstruct header file
#define MSG_LEN 1024         // Maximum size of a message to be exchanged between client and server
#define NICK_LEN 128         // Maximum length allowed for a client's nickname
#define MAX_CLIENTS 15       // Maximum number of clients that can connect simultaneously
#define MAX_EVENTS 2         // Maximum number of events to monitor (socket and stdin)
#define MSG_QUIT "/quit"     // Quit msg

// Colors definition
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// Server ip/port
#define SERV_PORT "8080"     // Server port number that listens for connections
#define SERV_ADDR "127.0.0.1"// Server IP address, here 'localhost' (the local machine)

// Client ip/port
#define CLIT_PORT "8081"
#define CLIT_ADDR "127.0.0.1"



////////////////////////// Structures //////////////////////////
// Structure to hold simple information
struct info {
    short s;
    long l;
};

typedef struct ClientInfo {
    int sockfd;               
    struct sockaddr_in address;  
    char nickname[NICK_LEN];   
    time_t connect_time;       
    char ip_address[INET_ADDRSTRLEN]; 
    u_short port_number;       
    struct ClientInfo *next; 
    char channel[CHAN_LEN];  
} ClientInfo;

typedef struct currentClientInfo {
    int sockfd;               
    struct sockaddr_in address;  
    char nickname[NICK_LEN];   
    time_t connect_time;       
    char ip_address[INET_ADDRSTRLEN]; 
    u_short port_number;        
    char channel[CHAN_LEN];
} currentClientInfo;

typedef struct Channel {
    char channel_name[CHAN_LEN];
    int activ_client;
    struct Channel *channel_next;
} Channel;

extern Channel *channel_list;





////////////////////////// Nickname Functions Prototypes //////////////////////////
int check_nickname(char *nickname);
void change_nickname(int sockfd, char *new_nickname, ClientInfo *clients_list);





////////////////////////// Messaging Functions Prototypes //////////////////////////
void handle_who(int sockfd, ClientInfo *clients_list);
void handle_whois(int sockfd, char *rqstnick, ClientInfo *clients_list);
void handle_echo(int sockfd, char *buff, int buff_len, char *nick_sender);
void handle_msg(int sockfd, char *buff, int buff_len, char *recipient_nickname, ClientInfo *clients_list, char *sender_nickname);
void handle_msgall(int sender_sockfd, char *buff, int buff_len, ClientInfo *clients_list);
void handle_whoami(int sockfd, char *rqstnick, ClientInfo *clients_list);





////////////////////////// Channel Functions prototypes //////////////////////////
int check_channel_name(char *channel); 
Channel* channel_info(Channel *c_list, char *c_name);
void addChannel(char *channel_name);
void destroy_channel(char *c_name);
void send_notification(char *channel_name, char *message, ClientInfo *client_list, char *sender_nickname);
void send_notification2(ClientInfo *client_list, char *channel_name, char *message, char *sender_nickname);
void handle_create(ClientInfo *client_list, char *nick_sender, char *channel_name);
void handle_join(ClientInfo *client_list, char *nick_sender, char *channel_name);
void handle_channel_list(ClientInfo *client_list, char *nickname_sender);
void handle_quit(ClientInfo *client_list, char *nickname_sender, char *channel_to_quit);
void handle_multicast(ClientInfo *client_list, char *nickname_sender, char *message, char *channel_name);





////////////////////////// File Functions prototypes //////////////////////////
void write_in_new_file(int sockfd, char *file_name, struct currentClientInfo *currentClient, char *buff, struct message msg);
void send_file(char *server_name, char *server_port, char *file_path, int socket_file);
void handle_file_reception(struct currentClientInfo *currentClient, char *sender_nick, char *file_name);
void send_file_request(ClientInfo *client_list, char *sender_nick, char *receiver_nick, char *file_name);
void respond_to_sender(ClientInfo *client_list, char *nick_sender, char *buffer_pld, struct message msg_response);





////////////////////////////////////// Command handling Functions Prototypes //////////////////////////////////////
void handle_message(char *input, struct message *msgstruct, char *nick_sender, char *buffer_pld, size_t buffer_size, struct currentClientInfo *currentClient);
void handle_command(int sockfd, struct message msgstruct, char *nick_sender, ClientInfo *clients_list, char *buff);
void unknown_command(ClientInfo *client_list, char *nickname_sender);





////////////////////////// Other Functions prototypes //////////////////////////
ClientInfo* sockfd_to_client(ClientInfo* list, int sockfd);
char* sockfd_to_nick(ClientInfo *list, int sockfd);
ClientInfo* nick_to_client(ClientInfo *list, char *nick);



#endif