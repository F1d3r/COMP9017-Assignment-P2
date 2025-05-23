#define _POSIX_C_SOURCE 200809L
#define MAX_CLIENTS 10

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>


volatile int interupted = 0;
pthread_t threads[MAX_CLIENTS];
int clinet_count = 0;

void* server_thread(void* arg){
    pid_t client_pid = *(pid_t*)arg;
    printf("Created a new thread for client: %d.\n", client_pid);

    // Send the response to the client.
    kill(client_pid, SIGRTMIN+1);

    // Free the client pid before terminate the thread.
    free(arg);
    return NULL;
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
        printf("Success created a thread.\n");
    }
}

void handle_SIGINT(int sig){
    interupted = 1;
}


int main(int argc, char *argv[]){
    // Server PID.  
    pid_t server_pid = getpid();
    printf("Server PID: %d.\n", server_pid);

    // Check arguments for the time interval.
    if(argc != 2){
        printf("Please input the server time interval as the argument.\n");
        return 1;
    }

    // Set interupt handler.
    signal(SIGINT, handle_SIGINT);

    // Get the server time interval.
    double server_time_interval = strtod(argv[1], NULL);
    printf("Got the time interval: %f.\n", server_time_interval);

    // Register SIGRTMIN handler by sigaction.
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = handle_SIGRTMIN;
    sigaction(SIGRTMIN, &sa, NULL);
    // Clear the signal set.
    sigemptyset(&sa.sa_mask);
    // Mask the SIGRTMIN during the signal handler processing.
    sigaddset(&sa.sa_mask, SIGRTMIN);


    // Creat the pipe to listen client connection.
    if(mkfifo("connection", 0666) < 0){
        perror("Failed to create pipe.\n");
    }

    int read_fd = open("connection", O_RDWR);

    // sigset_t mask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGRTMIN);
    while (!interupted) {
        pause();
    }


    // Join all threads.
    for(int i = 0; i < clinet_count; i++){
        pthread_join(threads[i], NULL);
    }


    close(read_fd);
    if (unlink("connection") < 0) {
        perror("Failed to remove pipe");
    }

    return 0;
}