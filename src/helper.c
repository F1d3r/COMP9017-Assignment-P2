#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "../libs/helper.h"


bool check_integer(char* str){
    if(!str || *str == '\0'){
        return false;
    }

    while(*str){
        if(!isdigit((unsigned char)*str)){
            return false;
        }
        str++;
    }

    return true;
}

void resolve_command(char* command_input, char** command, char** arg1, char** arg2, char** arg3){
        
    char* token = strtok(command_input, " ");
    *command = token;

    if(strcmp(*command, "DOC?\n") == 0){
        *arg1 = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "LOG?\n") == 0){
        *arg1 = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "PERM?\n") == 0){
        *arg1 = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "DISCONNECT\n") == 0){
        *arg1 = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "INSERT") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        token = strtok(NULL, "\n");
        *arg2 = token;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "DEL") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        token = strtok(NULL, "\n");
        *arg2 = token;
        *arg3 = NULL;
    }
    else{
        *arg1 = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
    }

}


bool check_command_insert(document* doc, char* arg1, char* arg2){
    if(arg2 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    uint64_t pos = 0;
    // Check if the position index an integer. 
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }else{
        pos = strtol(arg1, NULL, 10);
        printf("Got position: %ld.\n", pos);
    }
    // Check position validation.
    if(pos > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}

bool check_command_delete(document* doc, char* arg1, char* arg2){
    if(arg2 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t pos = 0;
    size_t len = 0;
    // Check if the position index an integer. 
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }
    else if(!check_integer(arg2)){
        printf("Invalid position index.\n");
        return false;
    }
    else{
        pos = strtol(arg1, NULL, 10);
        len = strtol(arg2, NULL, 10);
        printf("Got position: %ld.\n", pos);
        printf("Got length: %ld.\n", len);
    }
    // Check position validation.
    if(pos > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(pos + len > doc->doc_len){
        printf("Invalid length index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}