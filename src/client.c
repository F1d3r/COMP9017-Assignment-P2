#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>


bool conn_success = false;

void handle_SIGRTMIN(int sig){
    if(sig == SIGRTMIN+1){
        printf("Received SIGRTMIN+1.\n");
        conn_success = true;
    }else{
        printf("Runtime signal is not correctly responsed.\n");
        conn_success = false;
    }
}


int main(int argc, char *argv[]){
    // Check input arguments.
    long server_pid_value;
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
    }
    
    signal(SIGRTMIN+1, handle_SIGRTMIN);

    // Send SIGRTMIN to server.
    // Sleep for 1 second for response.
    pid_t server_pid = (pid_t)server_pid_value;
    pid_t pid = getpid();
    // Send its pid to the server as a signal value.
    union sigval value;
    value.sival_int = pid;

    sigqueue(server_pid, SIGRTMIN, value);
    sigqueue(server_pid, SIGRTMIN, value);
    sigqueue(server_pid, SIGRTMIN, value);
    sleep(1);
    if(!conn_success){
        printf("Server is not responding.\n");
        return 1;
    }

    // Connection established.
    printf("Successfully connected to the server.\n");
    return 0;

}