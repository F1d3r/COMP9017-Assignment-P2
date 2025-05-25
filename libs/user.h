#ifndef USER_H
#define USER_H

#include <pthread.h>

typedef struct{
    char* username;
    char* permission_level;
}User;

typedef struct {
    char* S2C_pipe_name;
    pid_t client_pid;
}Client;

void print_all_clients(Client** clients, int clients_count);

void add_client(Client*** clients, Client* client, int* clients_count);

void remove_client(Client*** clients, Client* client, int* clients_count);

void print_users(User** usres, int total_user);

void destroy_users(User** users, int total_user);

#endif