#include <stdlib.h>

#include "../libs/document.h"


void print_log(log* doc_log){
    log* current_log = doc_log;
    while(current_log != NULL){
        printf("VERSION %ld\n", current_log->version_num);
        for(int i = 0; i < doc_log->command_num; i++){
            printf("EDIT %s\n", doc_log->edits[i]);
        }
        printf("END\n");
        current_log = current_log->next_log;
    }
}

log* init_log(){
    log* log_head = (log*)malloc(sizeof(log));
    log_head->version_num = 0;
    log_head->command_num = 0;
    log_head->edits = NULL;
    log_head->next_log = NULL;

    return log_head;
}

void log_free(log* doc_log){
    log* current_log = doc_log;
    while(current_log != NULL){
        // Free all commands.
        for(int i = 0; i < current_log->command_num; i++){
            free(current_log->edits[i]);
        }
        // Free command array.
        if(current_log->edits != NULL){
            free(current_log->edits);
            current_log->edits = NULL;
        }
        log* temp = current_log;
        current_log = current_log->next_log;
        free(temp);
    }
}