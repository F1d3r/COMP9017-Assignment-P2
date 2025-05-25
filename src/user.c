#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/user.h"


// Print all clients in the array.
void print_all_clients(Client** clients, int clients_count){
    printf("All clients now:\n");
    for(int i = 0; i < clients_count; i++){
        printf("Client pid: %d\n", clients[i]->client_pid);
        printf("Pipe name: %s\n", clients[i]->S2C_pipe_name);
    }
}

// Add a client into the clients array.
void add_client(Client*** clients, Client* client, int* clients_count){
    // Extend the pipes mem allocation.
    *clients = realloc(*clients, sizeof(Client*)*(*clients_count+1));
    (*clients)[*clients_count] = client;
    (*clients_count)++;
}

// Remove a client from the clients array.
void remove_client(Client*** clients, Client* client, int* clients_count){
    for(int i = 0; i < *clients_count; i++){
        if((*clients)[i]->client_pid == client->client_pid){
            // Copy the clients before ith.
            free((*clients)[i]->S2C_pipe_name);
            free((*clients)[i]);
            memcpy((*clients)+i, (*clients)+i+1, sizeof(Client*)*(*clients_count-i-1));
            *clients = realloc(*clients, sizeof(Client*)*(*clients_count-1));
            break;
        }
    }
    (*clients_count)--;
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