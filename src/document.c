#include <stdlib.h>
#include <string.h>

#include "../libs/document.h"


void print_log(log* doc_log){
    log* current_log = doc_log;
    while(current_log != NULL){
        printf("VERSION %ld\n", current_log->version_num);
        for(int i = 0; i < doc_log->edits_num; i++){
            printf("EDIT %s\n", doc_log->edits[i]);
        }
        printf("END\n");
        current_log = current_log->next_log;
    }
}

log* init_log(){
    log* log_head = (log*)malloc(sizeof(log));
    log_head->version_num = 0;
    log_head->edits_num = 0;
    log_head->edits = NULL;
    log_head->next_log = NULL;

    return log_head;
}

// Add a new edit command to the log.
void add_edit(log** log_head, char* edit_content){
    // Go to the last log.
    log* last_log = *log_head;
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }
    char* new_edit = malloc(sizeof(char)*(strlen(edit_content)+1));
    last_log->edits = realloc(last_log->edits, sizeof(char*)*(last_log->edits_num+1));
    last_log->edits[last_log->edits_num] = new_edit;
    last_log->edits_num ++;
    return;
}

void add_log(log** log_head, log* new_log){
    log* current_log = *log_head;
    while(current_log->next_log != NULL){
        current_log = current_log->next_log;
    }
    current_log->next_log = new_log;
    return;
}


void log_free(log* doc_log){
    log* current_log = doc_log;
    while(current_log != NULL){
        // Free all commands.
        for(int i = 0; i < current_log->edits_num; i++){
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