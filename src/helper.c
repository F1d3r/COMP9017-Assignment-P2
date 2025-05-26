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
    else if(strcmp(*command, "BOLD") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        token = strtok(NULL, "\n");
        *arg2 = token;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "NEWLINE") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "HEADING") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        token = strtok(NULL, "\n");
        *arg2 = token;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "ITALIC") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        token = strtok(NULL, "\n");
        *arg2 = token;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "BLOCKQUOTE") == 0){
        token = strtok(NULL, " ");
        *arg1 = token;
        *arg2 = NULL;
        *arg3 = NULL;
    }
    else if(strcmp(*command, "CODE") == 0){
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


bool check_command_bold(document* doc, char* arg1, char* arg2){
    if(arg2 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t pos1 = 0;
    size_t pos2 = 0;
    // Check if the position index an integer. 
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }
    else if(!check_integer(arg2)){
        printf("Invalid position index.\n");
        return false;
    }

    pos1 = strtol(arg1, NULL, 10);
    pos2 = strtol(arg2, NULL, 10);
    printf("Got position start: %ld.\n", pos1);
    printf("Got position end: %ld.\n", pos2);

    if(pos1 >= pos2){
        printf("Invalid position index(pos1 > pos2).\n");
        return false;
    }
    // Check position validation.
    if(pos1 > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(pos2 > doc->doc_len){
        printf("Invalid length index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}


bool check_command_newline(document* doc, char* arg1){
    if(arg1 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t pos = 0;
    arg1[strlen(arg1)-1] = '\0';
    // Check if the position index an integer.
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }

    pos = strtol(arg1, NULL, 10);
    printf("Got position start: %ld.\n", pos);

    // Check position validation.
    if(pos > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}


bool check_command_heading(document* doc, char* arg1, char* arg2){
    if(arg1 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t level = 0;
    size_t pos = 0;
    // Check if the position index an integer.
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }
    if(!check_integer(arg2)){
        printf("Invalid position index.\n");
        return false;
    }

    level = strtol(arg1, NULL, 10);
    pos = strtol(arg2, NULL, 10);
    printf("Got position start: %ld.\n", pos);

    // Check position validation.
    if(level < 1 || level > 3){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(pos > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}


bool check_command_italic(document* doc, char* arg1, char* arg2){
    if(arg1 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t start = 0;
    size_t end = 0;
    // Check if the position index an integer.
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }
    if(!check_integer(arg2)){
        printf("Invalid position index.\n");
        return false;
    }

    start = strtol(arg1, NULL, 10);
    end = strtol(arg2, NULL, 10);
    printf("Got position start: %ld.\n", start);
    printf("Got position end: %ld.\n", end);

    // Check position validation.
    if(start > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(end > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(start >= end){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}


bool check_command_blockquote(document* doc, char* arg1){
    if(arg1 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t pos = 0;
    arg1[strlen(arg1)-1] = '\0';
    // Check if the position index an integer.
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }

    pos = strtol(arg1, NULL, 10);
    printf("Got position start: %ld.\n", pos);

    // Check position validation.
    if(pos > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}



bool check_command_code(document* doc, char* arg1, char* arg2){
    if(arg1 == NULL){
        printf("Invalid command.\n");
        return false;
    }
    size_t start = 0;
    size_t end = 0;
    // Check if the position index an integer.
    if(!check_integer(arg1)){
        printf("Invalid position index.\n");
        return false;
    }
    if(!check_integer(arg2)){
        printf("Invalid position index.\n");
        return false;
    }

    start = strtol(arg1, NULL, 10);
    end = strtol(arg2, NULL, 10);
    printf("Got position start: %ld.\n", start);
    printf("Got position end: %ld.\n", end);

    // Check position validation.
    if(start > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(end > doc->doc_len){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    if(start >= end){
        printf("Invalid position index(out of boundry).\n");
        return false;
    }
    printf("Valid argument.\n");
    return true;
}