#define _POSIX_C_SOURCE 200809L
#define BUFF_LEN 1024

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

volatile int interupted = 0;

void handle_SIGRTMIN(int sig){
    printf("Server responses. Establish connection.\n");
}

void handle_SIGINT(int sig){
    printf("Got interrupted.\n");
    interupted = 1;
}

// On receiving SIGUSR1. Means the server is down.
// Terminate the client.
void handle_SIGUSR1(int sig){
    interupted = 1;
}


int main(int argc, char *argv[]){
    // Check input arguments.
    long server_pid_value;
    char username[BUFF_LEN];
    char permission[BUFF_LEN];
    char buff[BUFF_LEN];
    bool authorised = false;
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
        }
    }

    // Handle SIGUSER1. The server told client to close.
    signal(SIGUSR1, handle_SIGUSR1);
    // Handle SIGINT.
    signal(SIGINT, handle_SIGINT);
    
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = handle_SIGRTMIN;
    sigaction(SIGRTMIN+1, &sa, NULL);

    // Send SIGRTMIN to server.
    // Sleep for 1 second for response.
    pid_t server_pid = (pid_t)server_pid_value;
    pid_t pid = getpid();
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

    // Read the permision return.
    read(read_fd, buff, BUFF_LEN);
    printf("Got response: %s.\n", buff);
    if(strcmp(buff, "Reject UNAUTHORISED") == 0){
        printf("Your identify is not authorised.\n");
        authorised = false;
    }else{
        strcpy(permission, buff);
        authorised = true;
    }


    // When the client is not interupted.
    while(!interupted && authorised){
        scanf("%s", buff);
        printf("Got a command: %s.\n", buff);
        if(strcmp(buff, "DISCONNECT") == 0){
            // Write the disconnect into pipe.
            printf("Writing to pipe.\n");
            write(write_fd, buff, strlen(buff));
            break;
        }else{
            printf("Invalid command.\n");
        }
    }

    
    // Tell the server.
    printf("Writing to pipe.\n");
    write(write_fd, "DISCONNECT", strlen("DISCONNECT"));


    close(read_fd);
    close(write_fd);
    unlink(fifo_name1);
    unlink(fifo_name2);
    return 0;

}