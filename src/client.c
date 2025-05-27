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
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

bool connecting = false;


void handle_SIGRTMIN(){
    // printf("Server responses. Establish connection.\n");
}

void handle_SIGINT(){
    printf("Got interrupted.\n");
    interupted = 1;
}



void* broadcast_thread_func(void* arg){
    // printf("Broadcast listener thread craeted.\n");
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

            // Got broadcast:
            // printf("Got broadcast:\n%s", buff);
            pthread_mutex_lock(&lock);
            // Resolve the broadcast to get a new log.
            log* new_log = get_log(buff);
            printf("Got new log:\n");
            print_log(new_log);

            // Check the number of edits before adding to log.
            if(new_log->edits_num != 0){
                // Add log.
                add_log(&(doc->log_head), new_log);
                // Then update the document.
                update_doc(doc);
                markdown_increment_version(doc);
            }
            // No edit(success/reject) made in this broadcast. 
            // No need to update log.
            else{
                if(new_log->edits != NULL){
                    free(new_log->edits);
                }
                free(new_log);
            }
            pthread_mutex_unlock(&lock);
                
            
        }
    }

    printf("Broadcast listener terminated.\n");
    return NULL;
}


int main(int argc, char *argv[]){
    long server_pid_value;
    char username[BUFF_LEN];
    char permission[BUFF_LEN];
    size_t version_num;
    size_t doc_len;
    char buff[BUFF_LEN];
    
    doc = markdown_init();
    pthread_t listen_broadcast;

    
    // Check input arguments.
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
            strcat(username, "\n");
            // printf("Got username: %s|", username);
            // printf("Got server pid: %ld|", server_pid_value);
        }
    }
    // Get the server and self pid.
    pid_t server_pid = (pid_t)server_pid_value;
    pid_t pid = getpid();

    // Handle SIGINT.
    signal(SIGINT, handle_SIGINT);
    // Register Runtime Signal handler.
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = handle_SIGRTMIN;
    sigaction(SIGRTMIN+1, &sa, NULL);

    // Send its pid to the server as a signal value.
    union sigval value;
    value.sival_int = pid;
    // Send the SIGRTMIN with client pid.
    sigqueue(server_pid, SIGRTMIN, value);
    pause();

    // Open the pipes to communicate.
    char fifo_name1[BUFF_LEN];
    char fifo_name2[BUFF_LEN];
    sprintf(fifo_name1, "FIFO_S2C_%d", pid);
    sprintf(fifo_name2, "FIFO_C2S_%d", pid);

    int write_fd = open(fifo_name2, O_WRONLY);
    if(write_fd == -1) {
        perror("Failed to open write FIFO");
        return 1;
    }
    // Write the username into the pipe
    write(write_fd, username, strlen(username));
    // printf("Username written.\n");

    int read_fd = open(fifo_name1, O_RDONLY);
    if(read_fd == -1) {
        perror("Failed to open read FIFO");
        close(write_fd);
        return 1;
    }

    
    // Now, read the permission, version number, 
    // document length and document from the pipe.
    char* document_content = NULL;

    // Read permission.
    memset(buff, 0, sizeof(buff));
    int pos = 0;
    char ch;
    while(read(read_fd, &ch, 1) > 0 && ch != '\n') {
        if(pos < BUFF_LEN - 1) {
            buff[pos++] = ch;
        }
    }
    buff[pos] = '\0';
    strcpy(permission, buff);
    // printf("Got permission: %s\n", permission);
    
    // Check permission
    if(strcmp(permission, "Reject UNAUTHORISED") == 0) {
        printf("Your identity is not authorised.\n");
        connecting = false;
    } 
    else {
        connecting = true;

        // Read the document version
        memset(buff, 0, sizeof(buff));
        pos = 0;
        while(read(read_fd, &ch, 1) > 0 && ch != '\n') {
            if(pos < BUFF_LEN - 1) {
                buff[pos++] = ch;
            }
        }
        buff[pos] = '\0';
        version_num = strtol(buff, NULL, 10);
        doc->version_num = version_num;
        doc->next_doc->version_num = version_num;
        doc->log_head->version_num = version_num;
        // printf("Got document version: %ld\n", version_num);

        // Read document length.
        memset(buff, 0, sizeof(buff));
        pos = 0;
        while(read(read_fd, &ch, 1) > 0 && ch != '\n') {
            if(pos < BUFF_LEN - 1) {
                buff[pos++] = ch;
            }
        }
        buff[pos] = '\0';
        doc_len = strtol(buff, NULL, 10);
        doc->doc_len = doc_len;
        // printf("Got document length: %ld\n", doc_len);

        // Read the document according to length.
        if(doc_len > 0) {
            document_content = malloc(doc_len + 1);
            if(document_content == NULL) {
                perror("Memory allocation failed");
                interupted = true;
            } else {
                ssize_t total_read = 0;
                ssize_t bytes_read;
                
                while(total_read < doc_len) {
                    bytes_read = read(read_fd, document_content + total_read, doc_len - total_read);
                    if(bytes_read <= 0) {
                        perror("Failed to read document content");
                        free(document_content);
                        document_content = NULL;
                        break;
                    }
                    total_read += bytes_read;
                }
                
                // If document content read.
                if(document_content != NULL) {
                    // Remove last '\n'
                    document_content[doc_len] = '\0';
                    // printf("Got document content (%ld bytes):\n%s\n", doc_len, document_content);
                    
                    if(doc->first_chunk != NULL){
                        if(doc->first_chunk->content != NULL){
                            free(doc->first_chunk->content);
                        }

                        doc->first_chunk->content = document_content;
                        doc->first_chunk->length = doc_len;
                    } else{
                        free(document_content);
                    }
                        
                }
            }
        } 
        else {
            // printf("Empty document.\n");
        }
    }

    // Create thread for listening broadcast.
    int result = pthread_create(&listen_broadcast, NULL, broadcast_thread_func, &read_fd);
    if(result != 0){
        perror("Thread create failed.\n");
        interupted = true;
    }else{
        // printf("Start listening command input.\n");
    }


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
            read(read_fd, dummy_buffer, 1);
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
            print_log(doc->log_head);
            printf("----------\n");
        }
        // INSERT
        else if(strcmp(command, "INSERT") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_insert(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // DELETE
        else if(strcmp(command, "DEL") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_delete(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // BOLD
        else if(strcmp(command, "BOLD") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_bold(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // NEWLINE
        else if(strcmp(command, "NEWLINE") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_newline(doc, arg1)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // NEWLINE
        else if(strcmp(command, "HEADING") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_heading(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // ITALIC
        else if(strcmp(command, "ITALIC") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_italic(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // BLOCKQUOTE
        else if(strcmp(command, "BLOCKQUOTE") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_blockquote(doc, arg1)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // CODE
        else if(strcmp(command, "CODE") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_code(doc, arg1, arg2)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // HORIZONTAL_RULE
        else if(strcmp(command, "HORIZONTAL_RULE") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_horizontal(doc, arg1)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));
        }
        // LINK
        if(strcmp(command, "LINK") == 0){
            // Check command argument validation.
            pthread_mutex_lock(&lock);
            if(!check_command_link(doc, arg1, arg2, arg3)){
                pthread_mutex_unlock(&lock);
                continue;
            }
            pthread_mutex_unlock(&lock);

            // printf("Command now: %s\n", command_input);
            write(write_fd, command_input, strlen(command_input));

        }
    }

    
    markdown_free(doc);
    close(read_fd);
    close(write_fd);
    unlink(fifo_name1);
    unlink(fifo_name2);
    return 0;

}