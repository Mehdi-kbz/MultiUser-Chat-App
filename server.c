#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include "common.h"
#include "msg_struct.h"
#include <ctype.h>

// Initialization of variables
int online_clients = 0;
ClientInfo *clientList;
Channel *channel_list = NULL;
ClientInfo *clientList = NULL;


////////////////////////////////////// User Functions //////////////////////////////////////
// Function to create a user
void add_user(ClientInfo **list, int sockfd, struct sockaddr_in address) {
    ClientInfo *new_user = (ClientInfo *)malloc(sizeof(ClientInfo));
    if (new_user == NULL) {
        perror("malloc");
        return;
    }
    new_user->sockfd = sockfd;
    new_user->address = address;
    memset(new_user->nickname, 0, NICK_LEN);
    new_user->connect_time = time(NULL);
    inet_ntop(AF_INET, &(address.sin_addr), new_user->ip_address, INET_ADDRSTRLEN);
    new_user->port_number = ntohs(address.sin_port);
    new_user->next = NULL;
    memset(new_user->channel, 0, CHAN_LEN);

    if (*list == NULL) {
        *list = new_user;
    } else {
        ClientInfo *current = *list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_user;
    }
}

// Function to delete a user
void remove_user(int sockfd) {
    ClientInfo *curr = clientList;
    ClientInfo *prev = NULL;

    while (curr != NULL) {
        if (curr->sockfd == sockfd) {
            if (prev == NULL) {
                clientList = curr->next;
            } else {
                prev->next = curr->next;
            }

            printf( ">> client" " %s"" with sockid number %d disconnected"  "\n", curr->nickname,sockfd-4);
            free(curr);
            online_clients--;

            return;
        }
        prev = curr;
        curr = curr->next;
    }

    printf("Client with sockfd: %d not found.\n", sockfd-4);
}





////////////////////////////////////// Command handling Functions //////////////////////////////////////
// Function to handle commands using switch case
void handle_command(int sockfd, struct message msgstruct, char *nick_sender, ClientInfo *clients_list, char *buff) {
    switch (msgstruct.type) {
        case UNKNOWN_COMMAND:
            unknown_command(clients_list, nick_sender);
            break;

        case NICKNAME_NEW:
            // Command to change nickname
            change_nickname(sockfd, msgstruct.infos, clients_list);
            break;

        case NICKNAME_LIST:
            // Command to list users
            handle_who(sockfd, clients_list);
            break;

        case NICKNAME_INFOS:
            // Command to get information about a user
            handle_whois(sockfd, msgstruct.infos, clients_list);
            break;

        case WHOAMI:
            // Command to get information about the current user
            handle_whoami(sockfd, msgstruct.nick_sender, clients_list);
            break;

        case ECHO_SEND:
            // Command to echo a message
            handle_echo(sockfd, buff, msgstruct.pld_len, nick_sender);
            break;

        case UNICAST_SEND:
            // Command to send a message to a specific user
            handle_msg(sockfd, buff, msgstruct.pld_len, msgstruct.infos, clients_list, nick_sender);
            break;

        case BROADCAST_SEND:
            // Command to send a message to all users
            handle_msgall(sockfd, buff, msgstruct.pld_len, clients_list);
            break;

        case MULTICAST_SEND:
            // Command to send a message to a specific group
            handle_multicast(clients_list, nick_sender, buff, msgstruct.infos);
            break;

        case MULTICAST_CREATE:
            // Command to create a group
            handle_create(clients_list, nick_sender, msgstruct.infos);
            break;

        case MULTICAST_QUIT:
            // Command to quit a group
            handle_quit(clients_list, nick_sender, msgstruct.infos);
            break;

        case MULTICAST_JOIN:
            // Command to join a group
            handle_join(clients_list, nick_sender, msgstruct.infos);
            break;

        case MULTICAST_LIST:
            // Command to list all groups
            handle_channel_list(clients_list, nick_sender);
            break;

        case FILE_REQUEST:
            // Command to request file sending (unimplemented)
             send_file_request(clients_list, msgstruct.nick_sender, msgstruct.infos, buff);
            break;

        case FILE_ACCEPT:
            // Command to handle file send acceptance (unimplemented)
            respond_to_sender(clients_list, msgstruct.infos, buff, msgstruct);
            break;

        case FILE_REJECT:
            // Command to handle file send rejection (unimplemented)
            respond_to_sender(clients_list, msgstruct.infos, buff, msgstruct);
            printf("%s" " refused file transfer.\n" , nick_sender);
            break;

        default:
            // Handle any unknown or unhandled command types
            printf("Unknown message type received.\n");
            break;
    }
}

// Function to handle unknown commands
void unknown_command(ClientInfo *client_list, char *nickname_sender) {
    ClientInfo *sender = nick_to_client(client_list, nickname_sender);
    if (!sender) {
        printf( "[Server]:Client not found.\n" );
        return;
    }

    struct message unknown_msg = { .type = UNKNOWN_COMMAND, .pld_len = strlen("Unknown command received") };
    char unknown_command[] = "Unknown command received";

    if (send(sender->sockfd, &unknown_msg, sizeof(unknown_msg), 0) <= 0 ||
        send(sender->sockfd, unknown_command, unknown_msg.pld_len, 0) <= 0) {
        printf( "[Server]: error sending unknown command message to client %s.\n" , sender->nickname);
        perror("send");
    }
}





////////////////////////////////////// Nickname Functions //////////////////////////////////////
// Function to update nickname
int update_nickname(char *nickname) {
    size_t nickname_len = strlen(nickname);
    ClientInfo *current = clientList;

    // Check nickname length
    if (nickname_len < 1 || nickname_len >= NICK_LEN) {
        printf( "%s"  " tried choosing a nickname with invalid length.\n" , current->nickname);
        return 3;
    }

    // Reserved name check
    if (strcasecmp(nickname, "Server") == 0) {
        printf("%s"" tried choosing 'server' as a nickname.\n" , current->nickname);
        return 0;
    }

    // Check if nickname already exists
    for (ClientInfo *current = clientList; current != NULL; current = current->next) {
        if (strcasecmp(nickname, current->nickname) == 0) {
            printf("%s"" tried choosing an already used nickname.\n" , current->nickname);
            return 0;
        }
    }

    // Validate characters in nickname
    
    for (size_t i = 0; i < nickname_len; i++) {
        if (!isalnum(nickname[i])) {
            printf( "%s"  " tried choosing a nickname with invalid characters.\n" , current->nickname);
            return 0;
        }
    }

    return 1; // Nickname is valid
}

// Function to handle nickname change
void change_nickname(int sockfd, char *new_nickname, ClientInfo *clients_list) {
    ClientInfo *current = clients_list;
    char old_nickname[NICK_LEN];

    while (current != NULL) {
        if (current->sockfd == sockfd) {
            strncpy(old_nickname, current->nickname, NICK_LEN);
            old_nickname[NICK_LEN - 1] = '\0';

            if (update_nickname(new_nickname) == 1) {
                strncpy(current->nickname, new_nickname, NICK_LEN);
                printf( "%s"" has changed their nickname to ""%s"".\n" , old_nickname, current->nickname);

                if (strlen(current->channel) > 0) {
                    char notif_message[MSG_LEN];
                    snprintf(notif_message, MSG_LEN, "%s has changed their nickname to %s", old_nickname, current->nickname);
                    send_notification(current->channel, notif_message, clients_list, current->nickname);
                }

                struct message nickname_msg = { .type = NICKNAME_SUCCESS, .pld_len = strlen("Nickname changed successfully") };
                strcpy(nickname_msg.nick_sender, current->nickname);
                strcpy(nickname_msg.infos, new_nickname);

                if (send(sockfd, &nickname_msg, sizeof(nickname_msg), 0) <= 0 || 
                    send(sockfd, "Nickname changed successfully", nickname_msg.pld_len, 0) <= 0) {
                    perror("send");
                }
            } 
            else {
                printf( "%s"" tried to change their nickname but failed.\n" , current->nickname);
                struct message nickname_msg = { .type = NICKNAME_ERROR, .pld_len = strlen("Failed to change the nickname") };
                strcpy(nickname_msg.nick_sender, current->nickname);
                strcpy(nickname_msg.infos, "Failed to change the nickname");

                if (send(sockfd, &nickname_msg, sizeof(nickname_msg), 0) <= 0 || 
                    send(sockfd, "Failed to change the nickname", nickname_msg.pld_len, 0) <= 0) {
                    perror("send");
                }
            }

            return;
        }
        current = current->next;
    }

    printf( " User not found.\n" );
}





////////////////////////////////////// Channel Functions //////////////////////////////////////
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

// Function to check if a channel exists
int check_channel_existence(ClientInfo *list, char *channel) {
    for (ClientInfo *curr = list; curr != NULL; curr = curr->next) {
        if (strcmp(curr->channel, channel) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to handle creating a new channel
void handle_create(ClientInfo *client_list, char *nick_sender, char *c_name) {
    ClientInfo *client = nick_to_client(client_list, nick_sender);
    struct message msgstruct;
    char payload[INFOS_LEN];

    // Check if the specified channel already exists
    int channel_exists = check_channel_existence(client_list, c_name);

    if (client != NULL) {
        char old_c_name[CHAN_LEN];
        strncpy(old_c_name, client->channel, CHAN_LEN - 1);
        old_c_name[CHAN_LEN - 1] = '\0';

        if (channel_exists == 0) {
            // Allocate memory for a new channel
            Channel *new_c = (Channel *)malloc(sizeof(Channel));
            if (new_c == NULL) {
                fprintf(stderr, "Memory allocation error for the new channel.\n");
                exit(EXIT_FAILURE);
            }
            // Initialize the new channel properties
            strncpy(new_c->channel_name, c_name, CHAN_LEN - 1);
            new_c->channel_name[CHAN_LEN - 1] = '\0';
            new_c->activ_client = 0;
            new_c->channel_next = NULL;
            // Add the new channel to the channel list
            if (channel_list == NULL) {
                channel_list = new_c;
            } else {
                Channel *current_c = channel_list;
                while (current_c->channel_next != NULL) {
                    current_c = current_c->channel_next;
                }
                current_c->channel_next = new_c;
            }
            // Update client and channel information
            Channel *channel = channel_info(channel_list, c_name);
            if (channel != NULL) {
                channel->activ_client++;
            }
            strncpy(client->channel, c_name, CHAN_LEN - 1);
            client->channel[CHAN_LEN - 1] = '\0';
            // Send success message to the client
            msgstruct.type = MULTICAST_CREATE_SUCCESS;
            strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
            msgstruct.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(payload, "Channel created successfully.", INFOS_LEN - 1);
            payload[INFOS_LEN - 1] = '\0';
            msgstruct.pld_len = strlen(payload);
            strncpy(msgstruct.infos, c_name, INFOS_LEN - 1);
            msgstruct.infos[INFOS_LEN - 1] = '\0';

            if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0 ||
                send(client->sockfd, payload, strlen(payload), 0) <= 0) {
                perror("send");
                printf( "Error: sending success message.\n" );
            }
            // Notify other users in the channel
            char notification_message[MSG_LEN];
            snprintf(notification_message, MSG_LEN, ">> Channel"  " %s"  " has been created by"  " %s." , c_name, nick_sender);
            handle_multicast(client_list, nick_sender, notification_message, c_name);

            // Notify the previous channel if the client left it
            if (strlen(old_c_name) > 0) {
                snprintf(notification_message, MSG_LEN, "Goodbye :)\n"  "[Server]:"  " %s left the channel.\n", nick_sender);
                send_notification(old_c_name, notification_message, client_list, nick_sender);
            }
            printf( "%s "  "created a new channel: %s.\n" , nick_sender, c_name);

        } else {
            // Channel already exists; send error message to client
            printf( "Error: "  "%s"  " tried creating a channel that already exists (%s).\n" , nick_sender, c_name);
            msgstruct.type = MULTICAST_CREATE_ERROR;
            strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
            msgstruct.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(payload, "Error creating channel. Channel already exists.", INFOS_LEN - 1);
            payload[INFOS_LEN - 1] = '\0';
            msgstruct.pld_len = strlen(payload);
            strncpy(msgstruct.infos, "", INFOS_LEN - 1);
            msgstruct.infos[INFOS_LEN - 1] = '\0';

            if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0 ||
                send(client->sockfd, payload, strlen(payload), 0) <= 0) {
                perror("send");
                printf( "Error: sending error message to the requester.\n" );
            }
        }
    } else {
        // Client not found; send error message
        printf( "Error: Client not found.\n" );
        msgstruct.type = MULTICAST_CREATE_ERROR;
        strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
        msgstruct.nick_sender[NICK_LEN - 1] = '\0';
        strncpy(payload, "Error creating channel. Client not found.", INFOS_LEN - 1);
        payload[INFOS_LEN - 1] = '\0';
        msgstruct.pld_len = strlen(payload);
        strncpy(msgstruct.infos, "", INFOS_LEN - 1);
        msgstruct.infos[INFOS_LEN - 1] = '\0';

        if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0 ||
            send(client->sockfd, payload, strlen(payload), 0) <= 0) {
            perror("send");
            printf( "Error: sending error message to the requester.\n" );
        }
    }
}

// Function to handle joining an existing channel
void handle_join(ClientInfo *client_list, char *nick_sender, char *channel_name) {
    ClientInfo *client = nick_to_client(client_list, nick_sender);
    struct message msg;
    char buffer_pld[MSG_LEN];

    if (client != NULL) {
        // Store the name of the old channel before joining the new channel
        char old_channel_name[CHAN_LEN];
        strncpy(old_channel_name, client->channel, CHAN_LEN - 1);
        old_channel_name[CHAN_LEN - 1] = '\0';

        if (check_channel_existence(client_list, channel_name)) {
            // Find the channel by name
            Channel *channel_to_join = channel_info(channel_list, channel_name);


            // Inform clients in the old channel (if applicable)
            char notification_message_new_2[MSG_LEN];
            
            if (strlen(old_channel_name) != 0) {
                snprintf(notification_message_new_2, MSG_LEN, "Goodbye :)\n""[Server]:"" %s left the channel.\n", nick_sender);
                handle_multicast(client_list, nick_sender, notification_message_new_2, old_channel_name);
            }

            // Add the client to the new channel
            strncpy(client->channel, channel_name, CHAN_LEN - 1);
            client->channel[CHAN_LEN - 1] = '\0';

            // Increment the value of activ_client for the new channel
            if (channel_to_join != NULL) {
                channel_to_join->activ_client++;
            }
            // Send a success message to the client
            msg.type = MULTICAST_JOIN_SUCCESS;
            strncpy(msg.nick_sender, "Server", NICK_LEN - 1);
            msg.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(msg.infos, channel_name, INFOS_LEN - 1);
            msg.infos[INFOS_LEN - 1] = '\0';

            snprintf(buffer_pld, MSG_LEN, "You joined channel %s.", channel_name);
            msg.pld_len = strlen(buffer_pld);

            if (send(client->sockfd, &msg, sizeof(struct message), 0) <= 0 ||
                send(client->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
                perror("send");
                printf( "Error: sending the success message.\n" );
            }

            // Notify clients in the new channel
            char notification_message_new_1[MSG_LEN];
            snprintf(notification_message_new_1, MSG_LEN, "Hello, i just joined this channel !\n");
            handle_multicast(client_list, nick_sender, notification_message_new_1, channel_name);


            printf("%s"" joined channel %s\n" , nick_sender, channel_name);
        } else {
            // Handling an invalid channel name
            msg.type = MULTICAST_JOIN_ERROR;
            strncpy(msg.nick_sender, "Server", NICK_LEN - 1);
            msg.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(msg.infos, "Invalid channel name. Use only letters and numbers.", INFOS_LEN - 1);
            msg.infos[INFOS_LEN - 1] = '\0';
            msg.pld_len = strlen(msg.infos);

            snprintf(buffer_pld, MSG_LEN, "Invalid channel name. Use only letters and numbers.");
            if (send(client->sockfd, &msg, sizeof(struct message), 0) <= 0 ||
                send(client->sockfd, buffer_pld, strlen(buffer_pld), 0) <= 0) {
                perror("send");
                printf( "Error: sending the error message to the client.\n" );
            }
        }
    } else {
        // Handling an unfound client
        printf( "Error: Client not found.\n" );
    }
}

// Function to list all available channels
void handle_channel_list(ClientInfo *client_list, char *nickname_sender) {
    ClientInfo *client = nick_to_client(client_list, nickname_sender);

    if (client != NULL) {
        int sockfd = client->sockfd;
        char channels[CHAN_LEN * 50] = {0}; // Buffer for channels list
        char added_channels[50][CHAN_LEN] = {0}; // Track added channels
        int unique_channel_count = 0;

        ClientInfo *current = client_list;

        while (current != NULL) {
            if (strlen(current->channel) > 0) {
                int already_added = 0;
                for (int i = 0; i < unique_channel_count; i++) {
                    if (strcmp(added_channels[i], current->channel) == 0) {
                        already_added = 1;
                        break;
                    }
                }

                if (!already_added) {
                    Channel *channel = channel_info(channel_list, current->channel);
                    int num_connected = (channel != NULL) ? channel->activ_client : 0;

                    snprintf(added_channels[unique_channel_count], CHAN_LEN, "%s", current->channel);
                    unique_channel_count++;

                    snprintf(channels + strlen(channels), CHAN_LEN * 50 - strlen(channels),
                             "  - %s (%d online%s)\n", current->channel, num_connected,
                             (strcmp(nickname_sender, current->nickname) == 0) ? ", me" : "");
                }
            }
            current = current->next;
        }

        char msg[MSG_LEN];
        snprintf(msg, MSG_LEN, " Active channels (%d):\n%s", unique_channel_count, 
                 unique_channel_count > 0 ? channels : " - no active channels.\n");

        struct message msgstruct = { .type = MULTICAST_LIST, .pld_len = strlen(msg) };
        strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
        msgstruct.nick_sender[NICK_LEN - 1] = '\0';
        msgstruct.infos[0] = '\0';

        if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0 ||
            send(sockfd, msg, strlen(msg), 0) <= 0) {
            perror("send");
            printf("[Server]: Error sending message to the requester.\n");
        }
    } else {
        printf( "Client not found.\n" );
    }
}

// Function to handle multicast messages
void handle_multicast(ClientInfo *client_list, char *nickname_sender, char *message, char *channel_name) {
    ClientInfo *sender = nick_to_client(client_list, nickname_sender);
    message[MSG_LEN - 1] = '\0';

    if (sender == NULL) {
        printf( "Error: Client not found.\n" );
        return;
    }

    // Verify sender is in the specified channel
   

    int success = 1; // Track if all sends are successful
    for (ClientInfo *current = client_list; current != NULL; current = current->next) {
        if (current != sender && strcmp(current->channel, channel_name) == 0) {
            struct message msg = { .type = MULTICAST_SEND };
            char buffer_pld[MSG_LEN];

            strncpy(msg.nick_sender, nickname_sender, NICK_LEN - 1);
            msg.nick_sender[NICK_LEN - 1] = '\0';
            msg.infos[0] = '\0';
            
            snprintf(buffer_pld, MSG_LEN,  "[%s]: "  "%s", nickname_sender, message);
            msg.pld_len = strlen(buffer_pld);

            if (send(current->sockfd, &msg, sizeof(msg), 0) <= 0 || send(current->sockfd, buffer_pld, strlen(buffer_pld), 0) <= 0) {
                perror("send");
                printf( "Error: sending message to client %s.\n" , current->nickname);
                success = 0;
            }
        }
    }

    // Send success or error notification back to the sender
    struct message response_msg;
    char response_pld[MSG_LEN];
    response_msg.type = success ? MULTICAST_SEND_SUCCESS : MULTICAST_SEND_ERROR;
    strncpy(response_msg.nick_sender, "Server", NICK_LEN - 1);
    response_msg.nick_sender[NICK_LEN - 1] = '\0';

    if (success) {
        strncpy(response_msg.infos, "Message sent successfully.", INFOS_LEN - 1);
        response_msg.infos[INFOS_LEN - 1] = '\0';
        snprintf(response_pld, MSG_LEN, "Your message was sent successfully.");
        printf( "%s"  " sent a multicast message to channel %s\n" , nickname_sender, channel_name);
    } else {
        strncpy(response_msg.infos, "Failed to send message.", INFOS_LEN - 1);
        response_msg.infos[INFOS_LEN - 1] = '\0';
        snprintf(response_pld, MSG_LEN, "There was an error sending your message.");
    }
    response_msg.pld_len = strlen(response_pld);

    if (send(sender->sockfd, &response_msg, sizeof(response_msg), 0) <= 0 ||
        send(sender->sockfd, response_pld, strlen(response_pld), 0) <= 0) {
        perror("send");
        printf( "Error: sending response to client %s.\n" , sender->nickname);
    }
}

// Funciton to quit a channel
void handle_quit(ClientInfo *client_list, char *nickname_sender, char *channel_to_quit) {
    ClientInfo *client = nick_to_client(client_list, nickname_sender);
    struct message msgstruct;
    char buffer_pld[INFOS_LEN];

    if (client != NULL) {
        Channel *channel_to_leave = channel_info(channel_list, channel_to_quit);

        if (channel_to_leave != NULL && strcmp(client->channel, channel_to_quit) == 0) {
            char multicast_message[MSG_LEN];

            snprintf(multicast_message, MSG_LEN, "Goodbye :)\n""[Server]:"" %s left the channel.\n", nickname_sender);
            handle_multicast(client_list, nickname_sender, multicast_message, channel_to_quit);

            if (channel_to_leave->activ_client == 1) {
                snprintf(multicast_message, MSG_LEN,  "[Server]:""you were the last user in the channel" " %s"  "\nchannel closed.\n", channel_to_leave->channel_name);
                send_notification2(client_list, channel_to_quit, multicast_message, nickname_sender);
                destroy_channel(channel_to_leave->channel_name);
            } else {
                channel_to_leave->activ_client--;
            }
            strncpy(client->channel, "", CHAN_LEN - 1);
            client->channel[CHAN_LEN - 1] = '\0';

            
            msgstruct.type = MULTICAST_QUIT_SUCCESS;
            strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
            msgstruct.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(msgstruct.infos, channel_to_quit, INFOS_LEN - 1);
            msgstruct.infos[INFOS_LEN - 1] = '\0';

            strncpy(buffer_pld, "You quit the channel successfully.", INFOS_LEN - 1);
            buffer_pld[INFOS_LEN - 1] = '\0';

            msgstruct.pld_len = strlen(buffer_pld);

            if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                perror("send");
                printf( "Error: sending success message to the client.\n" );
            } else if (send(client->sockfd, buffer_pld, msgstruct.pld_len, 0) <= 0) {
                perror("send");
                printf( "Error: sending success message content to the client.\n" );
            }
            printf( "%s"" left channel %s\n" , nickname_sender,channel_to_quit);
        } else {
            // Gestion d'un client n'étant pas dans le salon spécifié
            msgstruct.type = MULTICAST_QUIT_ERROR;
            strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
            msgstruct.nick_sender[NICK_LEN - 1] = '\0';
            strncpy(msgstruct.infos, "Error quitting the channel. Client is not in the specified channel.", INFOS_LEN - 1);
            msgstruct.infos[INFOS_LEN - 1] = '\0';

            strncpy(buffer_pld, "Error quitting the channel. Client is not in the specified channel.", INFOS_LEN - 1);
            buffer_pld[INFOS_LEN - 1] = '\0';

            msgstruct.pld_len = strlen(buffer_pld);

            
            if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                perror("send");
                printf( "Error: sending error message to the client.\n" );
            } else if (send(client->sockfd, buffer_pld, msgstruct.pld_len, 0) <= 0) {
                perror("send");
                printf( "Error: sending error message content to the client.\n" );
            }
        }
    } else {
        // Gestion d'un client introuvable
        printf( "Client not found.\n" );

        msgstruct.type = MULTICAST_QUIT_ERROR;
        strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
        msgstruct.nick_sender[NICK_LEN - 1] = '\0';
        strncpy(msgstruct.infos, "Client not found.", INFOS_LEN - 1);
        msgstruct.infos[INFOS_LEN - 1] = '\0';

        strncpy(buffer_pld, "Client not found.", INFOS_LEN - 1);
        buffer_pld[INFOS_LEN - 1] = '\0';

        msgstruct.pld_len = strlen(buffer_pld);
        if (send(client->sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
            perror("send");
        } else if (send(client->sockfd, buffer_pld, msgstruct.pld_len, 0) <= 0) {
            perror("send");
        }
    }
}

// Function to destroy a channel
void destroy_channel(char *c_name) {
    Channel *prev_c = NULL;
    Channel *curr_c = channel_list;

    while (curr_c != NULL) {
        if (strcmp(curr_c->channel_name, c_name) == 0) {
            if (prev_c == NULL) {
                channel_list = curr_c->channel_next;
            } 
            else {
                prev_c->channel_next = curr_c->channel_next;
            }
            free(curr_c);
            return;
        }
        prev_c = curr_c;
        curr_c = curr_c->channel_next;
    }
    fprintf(stderr, "Channel '%s' not found.\n", c_name);
}

// Function to find channel info using its name
Channel* channel_info(Channel *c_list, char *c_name) {
    Channel *curr = c_list;

    while (curr != NULL) {
        if (strcmp(curr->channel_name, c_name) == 0) {
            return curr;
        }
        curr = curr->channel_next;
    }
    return NULL;
}

// Function to send a notification to all clients in a specified channel
void send_notification(char *channel_name, char *message, ClientInfo *client_list, char *sender_nick) {
    // Locate the channel by name
    Channel *channel = channel_info(channel_list, channel_name);

    // If the channel does not exist, log an error message and exit
    if (channel == NULL) {
        printf("Channel '%s' does not exist.\n", channel_name);
        return;
    }

    struct message msgstruct;
    // Set the sender nickname and message type
    snprintf(msgstruct.nick_sender, NICK_LEN, "%s", sender_nick);
    msgstruct.type = MULTICAST_SEND;
    // Calculate and set the payload length, including the null terminator
    msgstruct.pld_len = strlen(message) + 1;
    // Set channel information in the message
    snprintf(msgstruct.infos, INFOS_LEN, "%s", channel_name);

    // Broadcast the message to all clients in the channel
    handle_multicast(client_list, sender_nick, message, channel_name);
}

// Function to notify all clients in a specific channel with a message
void send_notification2(ClientInfo *client_list, char *channel_name, char *message, char *sender_nick) {
    // Traverse the client list to find clients in the specified channel
    ClientInfo *current = client_list;
    while (current != NULL) {
        // Check if the client is in the target channel
        if (strcmp(current->channel, channel_name) == 0) {
            struct message msgstruct;
            // Set the sender nickname and message type
            snprintf(msgstruct.nick_sender, NICK_LEN, "%s", sender_nick);
            msgstruct.type = MULTICAST_SEND;
            // Include null terminator in payload length
            msgstruct.pld_len = strlen(message) + 1;
            // Store channel information in the message
            snprintf(msgstruct.infos, INFOS_LEN, "%s", channel_name);

            // Send the structured message to the client
            if (send(current->sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                perror("send");
                printf( "[Server]: --> Error sending multicast message to %s.\n" , current->nickname);
            }
            // Send the message content to the client
            else if (send(current->sockfd, message, msgstruct.pld_len, 0) <= 0) {
                perror("send");
                printf( "[Server]: --> Error sending multicast message content to %s.\n" , current->nickname);
            }
        }
        // Move to the next client in the list
        current = current->next;
    }
}





////////////////////////////////////// Messaging Functions //////////////////////////////////////
// Function to handle who message
void handle_who(int sockfd, ClientInfo *clients_list) {
    ClientInfo *curr = clients_list;
    char nlist[MSG_LEN] = "";

    char requester[NICK_LEN];
    strncpy(requester, sockfd_to_nick(clients_list, sockfd), NICK_LEN - 1);
    requester[NICK_LEN - 1] = '\0';

    int online_users = 0; 
    while (curr != NULL) {
        if (curr->sockfd != sockfd) {
            size_t remaining_space = MSG_LEN - strlen(nlist) - 1;
            size_t nlength = strlen(curr->nickname);

            if (nlength <= remaining_space) {
                strcat(nlist, "\n  - ");

                if (strcmp(curr->nickname, requester) == 0) {
                    strcat(nlist, curr->nickname);
                    strcat(nlist, " (me)");
                } else {
                    strcat(nlist, curr->nickname);
                }
                online_users++; 
            } else {
                break;
            }
        } else {
            size_t remaining_space = MSG_LEN - strlen(nlist) - 1; 
            size_t nlength = strlen(requester) + strlen(" (me)");

            if (nlength <= remaining_space) {
                strcat(nlist, "\n  - ");
                strcat(nlist, requester);
                strcat(nlist, " (me)");
                online_users++; 
                
            } else {
                break;
            }
        }
        curr = curr->next;
    }

    
    char count_clients_str[20];
    sprintf(count_clients_str, "(%d)", online_users);

    char msg[MSG_LEN] = " Online users ";
    if (online_users == 0) {
        strcat(msg, "(0 users): \n");
        strcat(msg, " - There are no connected users.\n");
    } else {
        strcat(msg, count_clients_str);
        strcat(msg, ":");
        strcat(msg, nlist);
    }

    struct message msgstruct;
    msgstruct.type = NICKNAME_LIST;
    strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
    msgstruct.nick_sender[NICK_LEN - 1] = '\0';
    msgstruct.pld_len = strlen(msg);
    strncpy(msgstruct.infos, "", INFOS_LEN - 1);
    msgstruct.infos[INFOS_LEN - 1] = '\0';


    if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
        perror("send");
    }

    if (send(sockfd, msg, strlen(msg), 0) <= 0) {
        perror("send");
    }

    printf( "List of connected users sent to ""%s.\n" , requester);
}

// Function to handle whois message
void handle_whois(int sockfd, char *rqstnick, ClientInfo *clients_list) {
    char buff_res[MSG_LEN];
    memset(buff_res, 0, MSG_LEN);

    ClientInfo *current = clients_list;
    int exists = 0; 

    while (current != NULL) {
        if (strcmp(current->nickname, rqstnick) == 0) {
            exists = 1;
            struct sockaddr_in clientAddr = current->address;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ip_str, INET_ADDRSTRLEN);
            snprintf(buff_res, MSG_LEN, "[Server]:"" %s"" connected since %s with IP address %s and port number %d\n", current->nickname, ctime(&current->connect_time), ip_str, ntohs(current->port_number));
            break;
        }
        current = current->next;
    }

    struct message msgstruct;
    msgstruct.type = NICKNAME_INFOS;
    strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
    msgstruct.nick_sender[NICK_LEN - 1] = '\0';

    if (exists) {
        msgstruct.pld_len = strlen(buff_res);
    } else {
        msgstruct.pld_len = 0;
        msgstruct.type = NICKNAME_INFOS_ERROR;
    }

    strncpy(msgstruct.infos, rqstnick, INFOS_LEN - 1);
    msgstruct.infos[INFOS_LEN - 1] = '\0';

    if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
        perror("send");
    }

    if (exists && send(sockfd, buff_res, strlen(buff_res), 0) <= 0) {
        perror("send");
    } else {
            printf("(whois) data sent to ""%s"".\n" , rqstnick);
        }  
}

// Function to handle echo replies
void handle_echo(int sockfd, char *buff, int buff_len, char *nick_sender) {
    struct message msgstruct;
    msgstruct.type = ECHO_SEND;
    strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
    msgstruct.nick_sender[NICK_LEN - 1] = '\0';
    msgstruct.pld_len = buff_len;
    strncpy(msgstruct.infos, "", INFOS_LEN - 1);
    msgstruct.infos[INFOS_LEN - 1] = '\0';

    if (buff_len > 0) {  
        
        if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
            perror("send");
        }

        if (send(sockfd, buff, buff_len, 0) <= 0) {
            perror("send");
        }
    }

    printf("echoed a message to %s.\n",nick_sender);
}

// Function to handle broadcasts
void handle_msgall(int sender_fd, char *message, int msg_length, ClientInfo *clients) {
    ClientInfo *node = clients;
    char payload_buffer[MSG_LEN];
    struct message feedback_msg;
    char sender_nickname[NICK_LEN];

    memset(sender_nickname, 0, NICK_LEN);

    // Retrieve sender's nickname
    while (node != NULL) {
        if (node->sockfd == sender_fd) {
            strncpy(sender_nickname, node->nickname, NICK_LEN - 1);
            break;
        }
        node = node->next;
    }

    // Broadcast message to all clients except sender
    node = clients;
    int transmission_failure = 0;

    while (node != NULL) {
        if (node->sockfd != sender_fd) {
            struct message broadcast_packet;
            memset(&broadcast_packet, 0, sizeof(struct message));

            broadcast_packet.type = BROADCAST_SEND;
            strncpy(broadcast_packet.nick_sender, sender_nickname, NICK_LEN - 1);
            broadcast_packet.pld_len = msg_length;
            memset(broadcast_packet.infos, 0, INFOS_LEN);

            // Send header
            if (send(node->sockfd, &broadcast_packet, sizeof(struct message), 0) <= 0) {
                transmission_failure = 1;
                fprintf(stderr, "[Error] Failed to send header to %s.\n", node->nickname);
            }

            // Send payload
            if (send(node->sockfd, message, msg_length, 0) <= 0) {
                transmission_failure = 1;
                fprintf(stderr, "[Error] Failed to send message to %s.\n", node->nickname);
            }

            printf("[Info] Broadcast sent to %s by %s.\n", node->nickname, sender_nickname);
        }
        node = node->next;
    }

    // Provide feedback to sender
    memset(&feedback_msg, 0, sizeof(struct message));
    if (transmission_failure) {
        feedback_msg.pld_len = strlen("Broadcast failed.");
        feedback_msg.type = BROADCAST_ERROR;
        strncpy(feedback_msg.nick_sender, sender_nickname, NICK_LEN - 1);
        strncpy(feedback_msg.infos, "Broadcast failed.", INFOS_LEN - 1);
        strncpy(payload_buffer, "Broadcast failed.", MSG_LEN - 1);

        fprintf(stderr, "[Warning] %s encountered errors during broadcast.\n", sender_nickname);
    } else {
        feedback_msg.pld_len = strlen("Broadcast successful.");
        feedback_msg.type = BROADCAST_SUCCESS;
        strncpy(feedback_msg.nick_sender, sender_nickname, NICK_LEN - 1);
        strncpy(feedback_msg.infos, "Broadcast successful.", INFOS_LEN - 1);
        strncpy(payload_buffer, "Broadcast successful.", MSG_LEN - 1);

        printf("[Success] %s successfully broadcasted a message.\n", sender_nickname);
    }

    if (send(sender_fd, &feedback_msg, sizeof(struct message), 0) <= 0) {
        perror("send feedback header");
    }
    if (send(sender_fd, payload_buffer, feedback_msg.pld_len, 0) <= 0) {
        perror("send feedback payload");
    }
}


// Function to handle unicast messages
void handle_msg(int sockfd, char *buff, int buff_len, char *recipient_nickname, ClientInfo *clients_list, char *s_nick) {
    int recipient_sockfd = -1;
    char buffer_pld[MSG_LEN];
    struct message msg_response;
    ClientInfo *current = clients_list;

    // Find recipient socket by nickname
    for (current = clients_list; current != NULL; current = current->next) {
        if (strcmp(current->nickname, recipient_nickname) == 0) {
            recipient_sockfd = current->sockfd;
            break;
        }
    }

    if (recipient_sockfd != -1) {
        // Send unicast message to recipient
        struct message msgstruct = {
            .type = UNICAST_SEND,
            .pld_len = buff_len
        };
        strncpy(msgstruct.nick_sender, s_nick, NICK_LEN - 1);
        msgstruct.nick_sender[NICK_LEN - 1] = '\0';
        strncpy(msgstruct.infos, recipient_nickname, INFOS_LEN - 1);
        msgstruct.infos[INFOS_LEN - 1] = '\0';

        if (send(recipient_sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0 ||
            send(recipient_sockfd, buff, buff_len, 0) <= 0) {
            perror("Error sending message to recipient");
            return;
        }
        printf(">> %s sent a unicast message to %s\n", s_nick, recipient_nickname);

        // Notify sender of successful unicast
        msg_response = (struct message) {
            .type = UNICAST_SUCCESS,
            .pld_len = strlen("Unicast message sent successfully")
        };
        strncpy(msg_response.nick_sender, s_nick, NICK_LEN - 1);
        msg_response.nick_sender[NICK_LEN - 1] = '\0';
        strncpy(buffer_pld, "Unicast message sent successfully", MSG_LEN);

    } else {
        // Notify sender that recipient was not found
        msg_response = (struct message) {
            .type = UNICAST_ERROR,
            .pld_len = strlen("Recipient not found")
        };
        strncpy(msg_response.nick_sender, s_nick, NICK_LEN - 1);
        msg_response.nick_sender[NICK_LEN - 1] = '\0';
        strncpy(buffer_pld, "Recipient not found", MSG_LEN);

        printf("Error: Recipient %s not found.\n" , recipient_nickname);
    }

    // Send response message to sender
    if (send(sockfd, &msg_response, sizeof(msg_response), 0) <= 0 ||
        send(sockfd, buffer_pld, msg_response.pld_len, 0) <= 0) {
        perror("Error sending response to sender");
    }
}

// Function to handle "whoami" message
void handle_whoami(int sockfd, char *rqstnick, ClientInfo *clients_list) {
    char buff_res[MSG_LEN];
    memset(buff_res, 0, MSG_LEN);

    // Find the client by nickname
    ClientInfo *sender = nick_to_client(clients_list, rqstnick);
    
    // Check if the client exists
    if (sender == NULL) {
        snprintf(buff_res, MSG_LEN,  "Client '%s' not found.\n" , rqstnick);
        
        // Prepare message structure for error
        struct message msgstruct;
        msgstruct.type = NICKNAME_INFOS_ERROR;
        strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
        msgstruct.nick_sender[NICK_LEN - 1] = '\0';
        msgstruct.pld_len = strlen(buff_res);
        strncpy(msgstruct.infos, rqstnick, INFOS_LEN - 1);
        msgstruct.infos[INFOS_LEN - 1] = '\0';

        // Send error message struct and payload
        send(sockfd, &msgstruct, sizeof(struct message), 0);
        send(sockfd, buff_res, msgstruct.pld_len, 0);
        return;
    }

    // If client is found, retrieve their information
    struct sockaddr_in clientAddr = sender->address;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip_str, INET_ADDRSTRLEN);
    snprintf(buff_res, MSG_LEN,  "[Server]: " "You are" " %s"  " connected since %s with IP address %s and port number %d\n",
             sender->nickname, ctime(&sender->connect_time), ip_str, ntohs(sender->port_number));
    
    // Prepare message structure for success
    struct message msgstruct;
    msgstruct.type = NICKNAME_INFOS;
    strncpy(msgstruct.nick_sender, "Server", NICK_LEN - 1);
    msgstruct.nick_sender[NICK_LEN - 1] = '\0';
    msgstruct.pld_len = strlen(buff_res);
    strncpy(msgstruct.infos, rqstnick, INFOS_LEN - 1);
    msgstruct.infos[INFOS_LEN - 1] = '\0';

    // Send message struct and payload
    if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
        perror("send");
    }
    if (send(sockfd, buff_res, strlen(buff_res), 0) <= 0) {
        perror("send");
    } else {
        if (rqstnick != NULL){
            printf("(whoami) data sent to ""%s"".\n" , rqstnick);
        }
    }
}





////////////////////////// File Functions  //////////////////////////
// Function to send a file sending request to the receiver
void send_file_request(ClientInfo *client_list, char *sender_nick, char *receiver_nick, char *file_name) {
    ClientInfo *receiver = nick_to_client(client_list, receiver_nick);
    ClientInfo *sender = nick_to_client(client_list, sender_nick); // Get the sender's client info
    char buffer_pld[MSG_LEN];
    struct message msg;

    // Check if the receiver exists
    if (receiver == NULL) {
        // Receiver does not exist; inform the sender
        msg.type = RECEIVER_EXISTENCE_ERROR;

        strncpy(msg.nick_sender, sender_nick, NICK_LEN - 1);
        msg.nick_sender[NICK_LEN - 1] = '\0';

        strncpy(msg.infos, sender_nick, INFOS_LEN - 1);
        msg.infos[INFOS_LEN - 1] = '\0';

        strncpy(buffer_pld, "Receiver not found.", MSG_LEN);
        buffer_pld[MSG_LEN - 1] = '\0';
        msg.pld_len = strlen(buffer_pld);

        // Send the error message to the sender
        if (send(sender->sockfd, &msg, sizeof(struct message), 0) <= 0) {
            perror("send");
            return;
        }

        if (send(sender->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
            perror("send");
            return;
        }

        // Log on the server side
        printf( "%s"" tried to send a file request to a non-existent user: %s.\n", sender_nick, receiver_nick);
        return;
    }

    // Check if the file exists
    if (access(file_name, F_OK) != 0) { // F_OK checks if the file exists
        // File does not exist; inform the sender
        msg.type = FILE_EXISTENCE_ERROR;

        strncpy(msg.nick_sender, sender_nick, NICK_LEN - 1);
        msg.nick_sender[NICK_LEN - 1] = '\0';

        strncpy(msg.infos, sender_nick, INFOS_LEN - 1);
        msg.infos[INFOS_LEN - 1] = '\0';

        strncpy(buffer_pld, "File not found.", MSG_LEN);
        buffer_pld[MSG_LEN - 1] = '\0';
        msg.pld_len = strlen(buffer_pld);

        // Send the error message to the sender
        if (send(sender->sockfd, &msg, sizeof(struct message), 0) <= 0) {
            perror("send");
            return;
        }

        if (send(sender->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
            perror("send");
            return;
        }

        // Log on the server side
        printf( "%s"  " tried to send a non-existent file: %s.\n", sender_nick, file_name);
        return;
    }

    // Proceed with sending the file request if both receiver and file exist
    msg.type = FILE_REQUEST;

    strncpy(msg.nick_sender, sender_nick, NICK_LEN - 1);
    msg.nick_sender[NICK_LEN - 1] = '\0';

    strncpy(msg.infos, receiver_nick, INFOS_LEN - 1);
    msg.infos[INFOS_LEN - 1] = '\0';

    strncpy(buffer_pld, file_name, MSG_LEN - 1);
    buffer_pld[MSG_LEN - 1] = '\0';
    msg.pld_len = strlen(buffer_pld);

    // Send file request to the receiver
    if (send(receiver->sockfd, &msg, sizeof(struct message), 0) <= 0) {
        perror("send");
    }

    if (send(receiver->sockfd, buffer_pld, msg.pld_len, 0) <= 0) {
        perror("send");
    }

    // Log the file request on the server side
    printf("%s"" sent a file sending request to %s.\n", sender_nick, receiver_nick);
}

void respond_to_sender(ClientInfo *client_list, char *nick_sender, char *buffer_pld, struct message msg_response) {
    // Find the recipient in the list of clients
    ClientInfo *sender = nick_to_client(client_list, nick_sender);

    if (sender != NULL) {
        // Send the file response to the recipient
        if (send(sender->sockfd, &msg_response, sizeof(struct message), 0) <= 0) {
                perror("send");
            }

        if (send(sender->sockfd, buffer_pld, msg_response.pld_len, 0) <= 0) {
                perror("send");
            }
    } else {
        printf("Recipient not found in the list of clients.\n");
    }
}





////////////////////////////////////// Other Functions //////////////////////////////////////
// Function to split a message
void split_message(const char *buff, char *result1, char *result2) {
    const char *msg1 = strchr(buff, ' ');

    if (msg1 != NULL) {
        msg1++;  // Move to the character after the first space
        const char *msg2 = strchr(msg1, ' ');

        if (msg2 != NULL) {
            msg2++;  // Move to the character after the second space
            strcpy(result2, msg2);
        }

        strncpy(result1, msg1, msg2 ? (size_t)(msg2 - msg1) : strlen(msg1));
    } else {
        strcpy(result1, " ");
    }
}

// Function to find a client using socket fd
ClientInfo* sockfd_to_client(ClientInfo* list, int sockfd) {
    ClientInfo* curr = list;
    while (curr != NULL) {
        if (curr->sockfd == sockfd) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL; 
}

// Function to find nickname using socket fd
char* sockfd_to_nick(ClientInfo *list, int sockfd) {
    ClientInfo *curr = list;
    while (curr != NULL) {
        if (curr->sockfd == sockfd) {
            return curr->nickname;
        }
        curr = curr->next;
    }
    return NULL;
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





////////////////////////////////////// Big Boss Functions //////////////////////////////////////
// Function to handle multiple clients
void handle_multiple_clients(int sfd) {
    struct pollfd fds[MAX_CLIENTS + 1];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    int nfds = 1;
    
    printf( "\nWaiting for connections");
    fflush(stdout);
    for (int i = 0; i < 3; ++i){
            printf(".");
            fflush(stdout);
            usleep(200000);
    }
    printf("\n");
       
    
    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret == -1) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int newsockfd = accept(sfd, (struct sockaddr *)&clientAddr, &clientLen);

            if (newsockfd < 0) {
                perror("accept");
                continue;
            }

            if (nfds < MAX_CLIENTS + 1) {
                add_user(&clientList, newsockfd, clientAddr);
                ClientInfo *new_user = sockfd_to_client(clientList, newsockfd);
                printf( "New client connected from ip"" %s"" and port"" %d"".",new_user->ip_address, new_user->port_number);
                printf( "\nWaiting for nicknames");
                fflush(stdout);
                for (int i = 0; i < 3; ++i){
                    printf(".");
                    fflush(stdout);
                    usleep(200000);
                }
                            

                online_clients++;

                fds[nfds].fd = newsockfd;
                fds[nfds].events = POLLIN;
                nfds++;
            } else {
                printf( "Error: Maximum number of clients reached, connection refused.\n");
                close(newsockfd);
            }
        }

        int messagesReceived = 0;

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                struct message msgstruct;
                char buff[MSG_LEN];

                // Memory cleanup
                memset(&msgstruct, 0, sizeof(struct message));
                memset(buff, 0, MSG_LEN);

                int bytesReceived = recv(fds[i].fd, &msgstruct, sizeof(struct message), 0);

                if (bytesReceived <= 0) {
                    remove_user(fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    ClientInfo *current = sockfd_to_client(clientList, fds[i].fd);

                    if (strlen(current->nickname) == 0) {
                        // If the client doesn't have a nickname, try to set the nickname
                        if (msgstruct.type == NICKNAME_NEW) {
                            char newNickname[NICK_LEN];
                            strncpy(newNickname, msgstruct.infos, NICK_LEN);
                            newNickname[NICK_LEN - 1] = '\0';

                            if (update_nickname(newNickname)) {
                                strcpy(current->nickname, newNickname);

                                // Display message when nickname is set
                                printf( "%s"" connected from ip"" %s"" and port ""%d"".",current->nickname, current->ip_address, current->port_number);

                                // Send success message to the client
                                struct message success_message;
                                success_message.type = NICKNAME_SUCCESS;
                                if (send(current->sockfd, &success_message, sizeof(struct message), 0) < 0) {
                                    perror("send");
                                    printf("\nError: Unable to send the message to the recipient.\n");
                                }
                            } else {
                                // Send error message to the client if the nickname already exists
                                struct message error_message_struct;
                                error_message_struct.type = NICKNAME_ERROR;
                                if (send(current->sockfd, &error_message_struct, sizeof(struct message), 0) < 0) {
                                    perror("send");
                                    printf("\nError: Unable to send the message to the recipient.\n");
                                }
                            }
                        }
                    } else {
                        // The client has a nickname, process commands as before
                        if (msgstruct.pld_len > 0) {
                            bytesReceived = recv(fds[i].fd, buff, msgstruct.pld_len, 0);
                        }

                        if (bytesReceived <= 0) {
                            remove_user(fds[i].fd);
                            close(fds[i].fd);
                            fds[i].fd = -1;
                        } else {
                            buff[bytesReceived] = '\0';
                            if (strncmp(buff, MSG_QUIT, strlen(MSG_QUIT)) == 0) {
                                remove_user(fds[i].fd);
                                close(fds[i].fd);
                                fds[i].fd = -1;
                            } else {
                                // If the client didn't send "/quit", process the command
                                handle_command(fds[i].fd, msgstruct, current->nickname, clientList, buff);
                                messagesReceived++;
                            }
                        }
                    }
                }
            }
        }

        if (nfds > 1 && messagesReceived == 0) {
            if (online_clients == 0) {
                printf( "\nWaiting for connections");
                fflush(stdout);
                for (int i = 0; i < 3; ++i){
                    printf(".");
                    fflush(stdout);
                    usleep(200000);
                }   
                printf("\n");       
            } 
            
            else {
                printf( "\nWaiting for messages");
                fflush(stdout);
                for (int i = 0; i < 3; ++i){
                    printf(".");
                    fflush(stdout);
                    usleep(200000);
                }        
                printf("\n");       
    
            }
        }
    }
}

// Function to bind the server to the specified port
int handle_bind(const char *port) {
    struct addrinfo hints, *result, *rp;
    int serverSocket;

    memset(&hints, 0, sizeof(struct addrinfo)); // Clear hints structure
    hints.ai_family = AF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP socket
    hints.ai_flags = AI_PASSIVE; // For wildcard IP address

    // Get address info for binding
    if (getaddrinfo(NULL, port, &hints, &result) != 0) {
        perror( "getaddrinfo()" );
        exit(EXIT_FAILURE);
    }

    // Iterate through the results and bind
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        serverSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (serverSocket == -1) {
            continue; // Try next address
        }

        // Set socket options
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Bind the socket to the address
        if (bind(serverSocket, rp->ai_addr, rp->ai_addrlen) == 0) {
            fprintf(stdout, "Server launched successfully on port %s.\n", port);
            break; // Successfully bound
        }

        close(serverSocket); // Close if bind failed
    }

    // If binding failed
    if (rp == NULL) {
        fprintf(stderr,  "Could not bind. Please try again." );
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); // Free the address info
    return serverSocket; // Return the bound socket
}

// Main Program 
int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf( "Missing arguments. Usage: %s <server_port>\n" , argv[0]);
        exit(EXIT_FAILURE);
    }

   const char *server_port = argv[1];
    int sfd; 
    sfd = handle_bind(server_port);

    if ((listen(sfd, SOMAXCONN)) != 0) {
        perror( "listen" );
        exit(EXIT_FAILURE);
    }

    handle_multiple_clients(sfd);

    close(sfd);
    return EXIT_SUCCESS;
}
