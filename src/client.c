#define _POSIX_C_SOURCE 200809L
#define BUFF_LEN 1024
#define CMD_LEN 256

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <ctype.h>

#include "../libs/markdown.h"
#include "../libs/document.h"
#include "../libs/helper.h"

volatile int interupted = 0;

document* doc;
log* doc_log;
pthread_mutex_t doc_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

bool connecting = false;


void handle_SIGRTMIN(int sig){
    printf("Server responses. Establish connection.\n");
}

void handle_SIGINT(int sig){
    printf("Got interrupted.\n");
    interupted = 1;
}

void* broadcast_thread_func(void* arg){
    printf("Broadcast listener thread craeted.\n");
    int read_fd = *(int*)arg;
    char buff[BUFF_LEN];

    // Using select to monitor the read_fd.
    fd_set readfds;
    struct timeval timeout;
    // Keep listening the broadcast message.
    while(!interupted){
        // Set timeout for checking interupted. Prevent blocking.
        timeout.tv_usec = 500000;
        timeout.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(read_fd, &readfds);

        int ret = select(read_fd+1, &readfds, NULL, NULL, &timeout);
        if(ret < 0){
            perror("Select failed.\n");
            break;
        }else if(ret == 0){
            // Timeouot.
            if(interupted){
                printf("Thread detected interruption.\n");
                break;
            }
            continue;
        }else if(FD_ISSET(read_fd, &readfds)){
            // There is data in the read_fd.
            // Read and handle the client requests from the pipe.
            memset(buff, 0, sizeof(buff));
            ssize_t bytes_read = read(read_fd, buff, BUFF_LEN);
            if(bytes_read == 0){
                connecting = false;
                break;
            } 
            else if(bytes_read < 0) {
                perror("Read error");
                connecting = false;
                break;
            }

            // printf("Got broadcast:\n%s.\n", buff);
            // Then resolve the broadcast message, 
            // to update the log, and local document.
            pthread_mutex_lock(&log_lock);
            log* new_log = get_log(buff);
            add_log(&doc_log, new_log);
            pthread_mutex_unlock(&log_lock);
            // Update doc.
            int num_edit_processed = update_doc(doc, doc_log);
            if(num_edit_processed != 0){
                markdown_increment_version(doc);
                new_log->version_num ++;
            }
        }
    }

    printf("Broadcast listener terminated.\n");
    return NULL;
}


int main(int argc, char *argv[]){
    // Check input arguments.
    long server_pid_value;
    char username[BUFF_LEN];
    char permission[BUFF_LEN];
    char buff[BUFF_LEN];
    
    doc = markdown_init();
    doc_log = init_log();

    pthread_t listen_broadcast;

    if(argc != 3){
        printf("Please provide the server PID and your username.\n");
        return 1;
    }else{
        char* end;
        server_pid_value = strtol(argv[1], &end, 10);
        if(*end != '\0' || server_pid_value <= 0){
            printf("Please provide positive integer to indicate server PID.");
            return 1;
        }
        
        if(strcmp(argv[2], "\n") == 0){
            printf("Please provide your username.\n");
            return 1;
        }else{
            strcpy(username, argv[2]);
            // username[strlen(argv[2])] = '\0';
            printf("Got username: %s\n", username);
            printf("Got server pid: %ld\n", server_pid_value);
        }
    }
    // Get the server and self pid.
    pid_t server_pid = (pid_t)server_pid_value;
    pid_t pid = getpid();

    // Handle SIGINT.
    signal(SIGINT, handle_SIGINT);
    // Register Runtime Signal handler.
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = handle_SIGRTMIN;
    sigaction(SIGRTMIN+1, &sa, NULL);
    // Send its pid to the server as a signal value.
    union sigval value;
    value.sival_int = pid;
    
    sigset_t signal_set;
    int sig;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGRTMIN+1);

    // Send the SIGRTMIN with client pid.
    sigqueue(server_pid, SIGRTMIN, value);
    sigwait(&signal_set, &sig);

    // Connection established.
    printf("Successfully connected to the server.\n");

    // Open the pipes to communicate.
    char fifo_name1[BUFF_LEN];
    char fifo_name2[BUFF_LEN];
    sprintf(fifo_name1, "FIFO_S2C_%d", pid);
    int read_fd = open(fifo_name1, O_RDONLY);
    sprintf(fifo_name2, "FIFO_C2S_%d", pid);
    int write_fd = open(fifo_name2, O_WRONLY);

    // Write the username into the pipe
    write(write_fd, username, strlen(username));

    // Read the server response.
    memset(buff, 0, sizeof(buff));
    read(read_fd, buff, BUFF_LEN);
    printf("Got response:\n%s|END OF MESSAGE\n", buff);
    if(strcmp(buff, "Reject UNAUTHORISED") == 0){
        printf("Your identify is not authorised.\n");
        connecting = false;
    }else{
        connecting = true;
    }

    // Resolve the response.
    // Get permission.
    char* token = strtok(buff, "\n");
    printf("Token: %p\n", token);
    strcpy(permission, token);
    printf("Got permission level: %s\n", permission);
    // Get document version.
    token = strtok(NULL, "\n");
    printf("Token: %p\n", token);
    doc->version_num = strtol(token, NULL, 10);
    printf("Got document version: %ld\n", doc->version_num);
    // Get document length.
    token = strtok(NULL, "\n");
    printf("Token: %p\n", token);
    doc->doc_len = strtol(token, NULL, 10);
    printf("Got document length: %ld\n", doc->doc_len);
    // Get document content.
    token = strtok(NULL, "\n");
    if(token != NULL){
        strcpy(doc->first_chunk->content, token);
        printf("Got document content: %s\n", doc->first_chunk->content);
    }else{
        printf("Content is empty: %s\n", doc->first_chunk->content);
    }


    // Create a thread to receive the server braodcast and update doc.
    int result = pthread_create(&listen_broadcast, NULL, broadcast_thread_func, &read_fd);
    if(result != 0){
        perror("Thread create failed.\n");
        interupted = true;
    }else{
        printf("Start listening command input.\n");
    }


    // When the client is not interupted.
    // Keeps listening from the user input for commands.
    while((!interupted) && connecting){
        char command_input[CMD_LEN];
        char temp[CMD_LEN];
        fgets(command_input, sizeof(command_input), stdin);
        strcpy(temp, command_input);

        // Resolve the command.
        char* command;
        char* arg1;
        char* arg2;
        char* arg3;
        resolve_command(temp, &command, &arg1, &arg2, &arg3);
        // printf("Got command: %s.\n", command);
        // printf("Got argument1: %s.\n", arg1);
        // printf("Got argument2: %s.\n", arg2);
        // printf("Got argument3: %s.\n", arg3);
        
        // Check the command formatting.
        // Make sure the command is valid formatting.
        // DISCONNECT
        if(strcmp(command, "DISCONNECT\n")==0){
            write(write_fd, command, CMD_LEN);
            interupted = true;
            // Waiting for server closing the pipe.
            char dummy_buffer[1];
            ssize_t result = read(read_fd, dummy_buffer, 1);
            continue;
        }
        // DOC
        else if(strcmp(command, "DOC?\n") == 0){
            markdown_print(doc, stdout);
            printf("----------\n");
        }
        // PERM
        else if(strcmp(command, "PERM?\n") == 0){
            printf("%s\n", permission);
            printf("----------\n");
        }
        // LOG
        else if(strcmp(command, "LOG?\n") == 0){
            print_log(doc_log);
            printf("----------\n");
        }
        // INSERT
        else if(strcmp(command, "INSERT") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&doc_lock);
            if(!check_command_insert(doc, arg1, arg2)){
                pthread_mutex_unlock(&doc_lock);
                continue;
            }
            pthread_mutex_unlock(&doc_lock);

            printf("Command now: %s\n", command_input);
            write(write_fd, command_input, CMD_LEN);
        }
        // DELETE
        else if(strcmp(command, "DEL") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&doc_lock);
            if(!check_command_delete(doc, arg1, arg2)){
                pthread_mutex_unlock(&doc_lock);
                continue;
            }
            pthread_mutex_unlock(&doc_lock);

            printf("Command now: %s\n", command_input);
            write(write_fd, command_input, CMD_LEN);
        }

        // LINK
        if(strcmp(command, "LINK") == 0){
            if(arg3 == NULL){
                printf("Invalid command.\n");
                continue;
            }

        }
    }

    



    markdown_free(doc);
    log_free(doc_log);
    close(read_fd);
    close(write_fd);
    unlink(fifo_name1);
    unlink(fifo_name2);
    return 0;

}