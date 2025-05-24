#define _POSIX_C_SOURCE 200809L
#define MAX_CLIENTS 10
#define BUFF_LEN 1024

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


typedef struct{
    char* username;
    char* permission_level;
}User;


// Global variables. Shared in all threads.
volatile int interupted = 0;
int total_user = 0;
int clinet_count = 0;

User** users;
document* doc;
log* doc_log;
pthread_mutex_t doc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_t threads[MAX_CLIENTS];

sem_t semaphore;



// The thread to handle client request.
void* server_thread(void* arg){
    // Resolve the client pid. As a local variable.
    char username[BUFF_LEN];
    char permission[BUFF_LEN];
    char command[BUFF_LEN];
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

    // Read the request from the fifo for the client.
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
            char* doc_content = markdown_flatten(doc);
            printf("Doc version: %ld\n", doc->version_num);
            printf("Doc len: %ld\n", doc->doc_len);
            printf("Doc content: %s\n", doc_content);

            // Write version number.
            snprintf(buff, sizeof(buff), "%s", permission);
            snprintf(buff+strlen(buff), sizeof(buff), "%lu\n", doc->version_num);
            snprintf(buff+strlen(buff), sizeof(buff), "%lu\n", doc->doc_len);
            snprintf(buff+strlen(buff), sizeof(buff), "%s", doc_content);
            printf("Message payload:%s |aaaaa.", buff);
            printf("|||");
            write(write_fd, buff, strlen(buff));

            free(doc_content);
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
            read(read_fd, command, BUFF_LEN);
            printf("Got command: %s from client: %d.\n", command, client_pid);
            // Resolve the command.
            char* token = strtok(command, " ");
            char* com = token;
            printf("Got command: %s.\n", com);
            token = strtok(NULL, " ");
            char* arg1 = token;
            printf("Got argument1: %s.\n", arg1);
            token = strtok(NULL, " ");
            char* arg2 = token;
            printf("Got argument2: %s.\n", arg2);
            token = strtok(NULL, " ");
            char* arg3 = token;
            printf("Got argument3: %s.\n", arg3);
            // If the command is disconnect.
            if(strcmp(command, "DISCONNECT\n") == 0){
                break;
            }
        }
        
    }


    // On connection lost.
    // If it is caused by server interuption.
    if(interupted){
        // Send an interupt signal to the serving client.
        printf("Sending SIGUSR1 to client: %d.\n", client_pid);
        kill(client_pid, SIGUSR1);
    }

    // Free the resources before terminate the thread.
    // Client pid.
    free(arg);
    // File descriptors.
    close(write_fd);
    close(read_fd);
    // FIFOs.
    unlink(fifo_name1);
    unlink(fifo_name2);
    printf("Thread %p for client: %d destroyed.\n", (void*)pthread_self(), client_pid);

    clinet_count--;
    pthread_exit(NULL);
}


void handle_SIGRTMIN(int sig, siginfo_t* info, void* context){
    // Check client amount.
    if(clinet_count >= MAX_CLIENTS){
        printf("Exceeded the maximum client amount.\n");
        return;
    }

    pid_t client_pid = info->si_pid;
    printf("Got a SIGRTMIN from %d.\n", client_pid);

    // Create a new thread for this client.
    pid_t* client_pid_ptr = malloc(sizeof(pid_t));
    *client_pid_ptr = client_pid;


    if(pthread_create(&threads[clinet_count], NULL, 
        server_thread, client_pid_ptr) != 0){
        // Handle thread craetion error.
        perror("Failed to create thread for the client.\n");
        free(client_pid_ptr);
    }else{
        clinet_count++;
        printf("Successfully created a thread.\n");
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
    double server_time_interval = strtod(argv[1], NULL);
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
            handle_SIGINT(SIGINT);
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