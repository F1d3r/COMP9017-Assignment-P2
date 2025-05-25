#define _POSIX_C_SOURCE 200809L
#define MAX_CLIENTS 10
#define BUFF_LEN 1024
#define CMD_LEN 256

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <semaphore.h>

#include "../libs/document.h"
#include "../libs/markdown.h"
#include "../libs/helper.h"


typedef struct{
    char* username;
    char* permission_level;
}User;

typedef struct {
    char* S2C_pipe_name;
    pid_t client_pid;
}Client;


// Global variables. Shared in all threads.
volatile int interupted = 0;
int total_user = 0;
int clinet_count = 0;
double server_time_interval;

User** users;
Client** clients;
document* doc;
log* doc_log;
pthread_mutex_t doc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t threads[MAX_CLIENTS];
pthread_t broadcast_thread;


void print_all_clients(Client** clients){
    pthread_mutex_lock(&clients_lock);
    for(int i = 0; i < clinet_count; i++){
        printf("Client pid: %d\n", clients[i]->client_pid);
        printf("Pipe name: %s\n", clients[i]->S2C_pipe_name);
    }

    pthread_mutex_unlock(&clients_lock);
}

void add_client(Client*** clients, Client* client){
    // Extend the pipes mem allocation.
    *clients = realloc(*clients, sizeof(Client*)*(clinet_count+1));
    *clients[clinet_count] = client;
    clinet_count++;
}

void remove_client(Client*** clients, Client* client){
    for(int i = 0; i < clinet_count; i++){
        if((*clients)[i]->client_pid == client->client_pid){
            // Copy the clients before ith.
            free((*clients)[i]->S2C_pipe_name);
            free((*clients)[i]);
            memcpy(**clients+i, **clients+i+1, sizeof(Client*)*(clinet_count-i-1));
            *clients = realloc(*clients, sizeof(Client*)*(clinet_count-1));
            break;
        }
    }
    clinet_count--;
}



// The thread to handle client request.
void* server_thread(void* arg){
    // Resolve the client pid. As a local variable.
    char username[BUFF_LEN];
    char permission[BUFF_LEN];
    char command_input[CMD_LEN];
    char buff[BUFF_LEN];
    bool connecting = true;
    pid_t client_pid = *(pid_t*)arg;

    printf("Created a new thread for client: %d.\n", client_pid);

    // Create fifos to communicate with the client.
    char fifo_name1[BUFF_LEN];
    char fifo_name2[BUFF_LEN];
    sprintf(fifo_name1, "FIFO_S2C_%d", client_pid);
    printf("S2C FIFO: %s.\n", fifo_name1);
    if(access(fifo_name1, F_OK) == 0){
        unlink(fifo_name1);
    }
    if(mkfifo(fifo_name1, 0666) < 0){
        perror("Failed to create pipe.\n");
    }
    // Add S2C pipe into array.

    sprintf(fifo_name2, "FIFO_C2S_%d", client_pid);
    printf("C2S FIFO: %s.\n", fifo_name2);
    if(access(fifo_name2, F_OK) == 0){
        unlink(fifo_name2);
    }
    if(mkfifo(fifo_name2, 0666) < 0){
        perror("Failed to create pipe.\n");
    }

    // Send the response to the client.
    kill(client_pid, SIGRTMIN+1);
    printf("Connection establish response sent to the client: %d.\n", client_pid);

    // Open the pipes to communicate with the client.
    int write_fd = open(fifo_name1, O_WRONLY);
    int read_fd = open(fifo_name2, O_RDONLY);
    printf("Thread listening on %d.\n", read_fd);
    printf("Thread writing on %d.\n", write_fd);

    // Read the username written by the client.
    read(read_fd, username, BUFF_LEN);
    printf("Got username from pipe: %s.\n", username);

    // Verify user.
    bool user_exist = false;
    for(int i = 0; i < total_user; i++){
        // If find a match.
        if(strcmp(users[i]->username, username) == 0){
            // Get the permission level of the matched user.
            user_exist = true;
            strcpy(permission, users[i]->permission_level);
            permission[strlen(permission)] = '\n';
            permission[strlen(permission)] = '\0';

            // Write document details to the pipe.
            pthread_mutex_lock(&doc_lock);
            // Check document.
            // char* doc_content = markdown_flatten(doc);
            printf("Doc version: %ld\n", doc->version_num);
            printf("Doc len: %ld\n", doc->doc_len);
            printf("Doc content: %s\n", doc->first_chunk->content);

            // Write version number.
            snprintf(buff, sizeof(buff), "%s", permission);
            snprintf(buff+strlen(buff), sizeof(buff), "%lu\n", doc->version_num);
            snprintf(buff+strlen(buff), sizeof(buff), "%lu\n", doc->doc_len);
            snprintf(buff+strlen(buff), sizeof(buff), "%s", doc->first_chunk->content);
            printf("Message payload:\n%s", buff);
            write(write_fd, buff, strlen(buff));

            // free(doc_content);
            pthread_mutex_unlock(&doc_lock);
            break;
        }
    }
    // If the username does not exist.
    if(!user_exist){
        printf("The user: %s does not exists in the permission list.\n", username);
        // Write the rejection into pipe.
        write(write_fd, "Reject UNAUTHORISED", strlen("Reject UNAUTHORISED"));
        connecting = false;
        sleep(1);
    }

    // Create a new client.
    Client* client = malloc(sizeof(Client));
    client->client_pid = client_pid;
    client->S2C_pipe_name = malloc(sizeof(char)*(strlen(fifo_name1)+1));
    strcpy(client->S2C_pipe_name, fifo_name1);
    add_client(&clients, client);
    print_all_clients(clients);

    


    // Using select to monitor the read_fd.
    fd_set readfds;
    struct timeval timeout;
    while(connecting){
        // Set timeout for checking interupted. Prevent blocking.
        timeout.tv_usec = 500000;
        timeout.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(read_fd, &readfds);

        int ret = select(read_fd + 1, &readfds, NULL, NULL, &timeout);

        if(ret < 0){
            perror("Select failed.\n");
            break;
        }else if(ret == 0){
            // Timeouot.
            if(interupted){
                printf("Thread %d detected interruption\n", client_pid);
                break;
            }
            continue;
        }else if(FD_ISSET(read_fd, &readfds)){
            // There is data in the read_fd.
            // Read and handle the client requests from the pipe.
            memset(command_input, 0, CMD_LEN);
            read(read_fd, command_input, CMD_LEN);
            printf("Got command: %s from client: %d.\n", command_input, client_pid);
            char temp[CMD_LEN];
            strcpy(temp, command_input);

            // Resolve the command.
            char* command;
            char* arg1;
            char* arg2;
            char* arg3;
            resolve_command(temp, &command, &arg1, &arg2, &arg3);

            // If the command is disconnect.
            if(strcmp(command, "DISCONNECT\n") == 0){
                connecting = false;
                break;
            }else if(strcmp(command, "INSERT") == 0){
                char edit_message[BUFF_LEN];
                // Remove the last '\n';
                command_input[strlen(command_input)-1] = '\0';
                sprintf(edit_message, "%s", command_input);

                // Check permission.
                if(strcmp(permission, "write\n") != 0){
                    // Unauthorised.
                    sprintf(edit_message+strlen(edit_message), " Reject UNAUTHORISED");
                    pthread_mutex_lock(&log_lock);
                    add_edit(&doc_log, edit_message);
                    pthread_mutex_unlock(&log_lock);
                    printf("No permission.\n");
                    continue;
                }

                // Check position
                uint64_t pos = strtol(arg1, NULL, 10);
                pthread_mutex_lock(&log_lock);
                log* last_log = doc_log;
                while(last_log->next_log != NULL){
                    last_log = last_log->next_log;
                }
                if(pos > doc->doc_len){
                    // Out of boundry.
                    sprintf(edit_message+strlen(edit_message), " Reject INVALID_POSITION");
                    add_edit(&doc_log, edit_message);
                    pthread_mutex_unlock(&log_lock);
                    printf("Invalid position.\n");
                    continue;
                }
                pthread_mutex_unlock(&log_lock);
                printf("The command is valid!\n");


                // Now the argument is valid, insert to the document.
                pthread_mutex_lock(&doc_lock);
                markdown_insert(doc, doc->version_num, pos, arg2);
                markdown_print(doc, stdout);
                pthread_mutex_unlock(&doc_lock);

                // Then write the command into log.
                sprintf(edit_message+strlen(edit_message), " SUCCESS");
                pthread_mutex_lock(&log_lock);
                add_edit(&doc_log, edit_message);
                pthread_mutex_unlock(&log_lock);
            }
        }
        
    }


    // On connection lost.
    // If it is caused by server interuption.
    if(interupted){
        // Send an interupt signal to the serving client.
        // printf("Sending SIGUSR1 to client: %d.\n", client_pid);
        kill(client_pid, SIGUSR1);
    }

    // Free the resources before terminate the thread.
    // Remove client.
    remove_client(&clients, client);
    // Client pid.
    free(arg);
    // File descriptors.
    close(write_fd);
    close(read_fd);
    // FIFOs.
    // Remove S2C pipe from array.
    unlink(fifo_name1);
    unlink(fifo_name2);
    // printf("Thread %p for client: %d destroyed.\n", (void*)pthread_self(), client_pid);

    
    pthread_exit(NULL);
}


void handle_SIGRTMIN(int sig, siginfo_t* info, void* context){
    // Check client amount.
    if(clinet_count >= MAX_CLIENTS){
        printf("Exceeded the maximum client amount.\n");
        return;
    }

    pid_t client_pid = info->si_pid;
    // printf("Got a SIGRTMIN from %d.\n", client_pid);

    // Create a new thread for this client.
    pid_t* client_pid_ptr = malloc(sizeof(pid_t));
    *client_pid_ptr = client_pid;

    // TODO Change the threads array logic here.
    // TODO Change the threads array logic here.
    // TODO Change the threads array logic here.
    // TODO Change the threads array logic here.
    if(pthread_create(&threads[clinet_count], NULL, 
        server_thread, client_pid_ptr) != 0){
        // Handle thread craetion error.
        perror("Failed to create thread for the client.\n");
        free(client_pid_ptr);
    }else{
        // printf("Successfully created a thread.\n");
    }
}

// On interupt or QUIT calls, clear all threads.
void handle_SIGINT(int sig){
    interupted = 1;
}


void print_users(User** usres, int total_user){
    for(int i = 0; i < total_user; i ++){
        printf("User %d:\n", i);
        printf("Username: %s.\n", usres[i]->username);
        printf("Permission: %s.\n", usres[i]->permission_level);
    }
}

void destroy_users(User** users, int total_user){
    for(int i = 0; i < total_user; i ++){
        free(users[i]->username);
        free(users[i]->permission_level);
        free(users[i]);
    }
    free(users);
}

void broadcast_to_all_clients(const char* message) {
    for(int i = 0; i < clinet_count; i++) {
        if(clients[i] != NULL && clients[i]->S2C_pipe_name != NULL) {
            int write_fd = open(clients[i]->S2C_pipe_name, O_WRONLY | O_NONBLOCK);
            if(write_fd >= 0) {
                write(write_fd, message, strlen(message));
                close(write_fd);
                printf("Broadcasted to client %d via %s\n", 
                       clients[i]->client_pid, clients[i]->S2C_pipe_name);
            } else {
                printf("Failed to open pipe %s for client %d\n", 
                       clients[i]->S2C_pipe_name, clients[i]->client_pid);
            }
        }
    }
}


void* broadcast_thread_func(void* arg) {
    while(!interupted) {
        // Set a timer.
        sleep((int)server_time_interval);
        
        // Check server status.
        if(interupted) break;
        
        // Build the message to broadcast.
        char broadcast_message[BUFF_LEN];
        pthread_mutex_lock(&log_lock);
        // Made a new log.
        log* new_log = init_log();
        log* last_log = doc_log;
        while(last_log->next_log != NULL){
            last_log = last_log->next_log;
        }
        new_log->version_num = last_log->version_num+1;
        // Record version length.
        pthread_mutex_lock(&doc_lock);
        last_log->current_ver_len = doc->doc_len;
        pthread_mutex_unlock(&doc_lock);
        // Add the new log into the log list.
        add_log(&doc_log, new_log);

        // Prepare the message.
        sprintf(broadcast_message, "Version %ld\n", last_log->version_num);
        for(int i = 0; i < last_log->edits_num; i++){
            sprintf(broadcast_message+strlen(broadcast_message), "Edit %s\n", last_log->edits[i]);
        }
        sprintf(broadcast_message+strlen(broadcast_message), "END\n");
        printf("Payload:\n%s", broadcast_message);

        pthread_mutex_unlock(&log_lock);

        // Broadcast to clients.
        broadcast_to_all_clients(broadcast_message);
        printf("Broadcasted log to all clients\n");
    }
    return NULL;
}


int main(int argc, char *argv[]){
    // Initialize the document.
    doc = markdown_init();
    doc_log = init_log();
    markdown_print(doc, stdout);

    // Server PID.  
    pid_t server_pid = getpid();
    printf("Server PID: %d.\n", server_pid);

    // Check arguments for the time interval.
    if(argc != 2){
        printf("Please input the server time interval as the argument.\n");
        return 1;
    }

    // Get the server time interval.
    server_time_interval = strtod(argv[1], NULL);
    printf("Got the time interval: %f.\n", server_time_interval);


    // Setup the valid users.
    FILE* roles = fopen("../roles.txt", "r");
    if(!roles){
        perror("Failed to open role file.\n");
        return 1;
    }
    // Initialize the users.
    char username_buff[BUFF_LEN];
    char permission_buff[BUFF_LEN];
    while(fscanf(roles, "%s %s", username_buff, permission_buff) == 2){
        printf("Valid username: %s.\n", username_buff);
        printf("Permission level: %s.\n", permission_buff);
        // Create a new user.
        User* newUser = malloc(sizeof(User));
        newUser->username = malloc(sizeof(char)*(strlen(username_buff)+1));
        newUser->permission_level = malloc(sizeof(char)*(strlen(permission_buff)+1));
        strcpy(newUser->username, username_buff);
        strcpy(newUser->permission_level, permission_buff);

        // Add the new user into users.
        users = realloc(users, sizeof(User)*(total_user+1));
        users[total_user] = newUser;
        total_user ++;
    }

    // Create thread for broadcasting.
    if(pthread_create(&broadcast_thread, NULL, broadcast_thread_func, NULL) != 0) {
        perror("Failed to create timer thread");
        return 1;
    }


    // Set interupt handler.
    signal(SIGINT, handle_SIGINT);
    // Register SIGRTMIN handler by sigaction.
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_SIGRTMIN;
    sigaction(SIGRTMIN, &sa, NULL);

    // // Clear the signal set.
    // sigemptyset(&sa.sa_mask);
    // // Mask the SIGRTMIN during the signal handler processing.
    // sigaddset(&sa.sa_mask, SIGRTMIN);


    // sigset_t mask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGRTMIN);
    char input_buff[BUFF_LEN];
    while (!interupted) {
        fgets(input_buff, sizeof(input_buff), stdin);
        if(strcmp(input_buff, "QUIT\n") == 0){
            if(clinet_count != 0){
                printf("QUIT rejected, %d clients still connected.\n", clinet_count);
            }else{
                handle_SIGINT(SIGINT);
            }
        }else if(strcmp(input_buff, "DOC?\n") == 0){
            pthread_mutex_lock(&doc_lock);
            markdown_print(doc, stdout);
            pthread_mutex_unlock(&doc_lock);
        }else if(strcmp(input_buff, "LOG?\n") == 0){
            pthread_mutex_lock(&log_lock);
            print_log(doc_log);
            pthread_mutex_unlock(&log_lock);
        }else{
            printf("Invalid command.\n");
        }
    }


    // Join all threads.
    printf("Not terminated threads: %d.\n", clinet_count);
    for(int i = 0; i < clinet_count; i++){
        printf("Waiting for thread: %p.\n", (void*)threads[i]);
        pthread_join(threads[i], NULL);
    }


    // Free the users.
    destroy_users(users, total_user);
    markdown_free(doc);
    log_free(doc_log);

    return 0;
}