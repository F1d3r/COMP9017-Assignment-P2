#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

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
            token = strtok(NULL, "\0");
            *arg2 = token;
            *arg3 = NULL;
        }else{
            *arg1 = NULL;
            *arg2 = NULL;
            *arg3 = NULL;
        }
}
