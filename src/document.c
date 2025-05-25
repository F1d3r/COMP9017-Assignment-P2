#include <stdlib.h>
#include <string.h>

#include "../libs/document.h"


void print_log(log* doc_log){

    log* current_log = doc_log;
    while(current_log != NULL){
        printf("VERSION %ld\n", current_log->version_num);
        for(int i = 0; i < current_log->edits_num; i++){
            printf("EDIT %s %s %s", current_log->edits[i]->user, 
                current_log->edits[i]->command, 
                current_log->edits[i]->result);
            if(current_log->edits[i]->reject_reason != NULL){
                printf(" %s\n", current_log->edits[i]->reject_reason);
            }else{
                printf("\n");
            }
        }
        printf("END\n");
        current_log = current_log->next_log;
    }
}


log* init_log(){
    log* log_head = (log*)malloc(sizeof(log));
    log_head->version_num = 0;
    log_head->current_ver_len = 0;
    log_head->edits_num = 0;
    log_head->edits = NULL;
    log_head->next_log = NULL;

    return log_head;
}

// Add a new edit command to the log.
void add_edit(log** log_head, char* user, char* command, char* result, char* reject_reason){
    // Go to the last log.
    log* last_log = *log_head;
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }
    edit* new_edit = malloc(sizeof(edit));
    new_edit->user = malloc(sizeof(char)*(strlen(user)+1));
    new_edit->command = malloc(sizeof(char)*(strlen(command)+1));
    new_edit->result = malloc(sizeof(char)*(strlen(result)+1));
    strcpy(new_edit->user, user);
    strcpy(new_edit->command, command);
    strcpy(new_edit->result, result);
    if(reject_reason != NULL){
        new_edit->reject_reason = malloc(sizeof(char)*(strlen(reject_reason)+1));
        strcpy(new_edit->reject_reason, reject_reason);
    }else{
        new_edit->reject_reason = NULL;
    }
    // Now insert the new edit into the array.

    last_log->edits = realloc(last_log->edits, sizeof(edit*)*(last_log->edits_num+1));
    last_log->edits[last_log->edits_num] = new_edit;
    last_log->edits_num ++;
    return;
}

void add_log(log** log_head, log* new_log){
    log* last_log = *log_head;
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }
    last_log->next_log = new_log;
    return;
}


void log_free(log* doc_log){
    log* current_log = doc_log;
    while(current_log != NULL){
        // Free all commands.
        for(int i = 0; i < current_log->edits_num; i++){
            free(current_log->edits[i]->user);
            current_log->edits[i]->user = NULL;
            free(current_log->edits[i]->command);
            current_log->edits[i]->command = NULL;
            free(current_log->edits[i]->result);
            current_log->edits[i]->result = NULL;
            if(current_log->edits[i]->reject_reason != NULL){
                free(current_log->edits[i]->reject_reason);
                current_log->edits[i]->reject_reason = NULL;
            }
            free(current_log->edits[i]);
            current_log->edits[i] = NULL;
        }
        // Free command array.
        if(current_log->edits != NULL){
            free(current_log->edits);
            current_log->edits = NULL;
        }
        // Free the current log.
        log* temp = current_log;
        current_log = current_log->next_log;
        free(temp);
    }
}