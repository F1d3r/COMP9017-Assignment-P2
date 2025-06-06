#include <stdlib.h>
#include <string.h>

#include "../libs/document.h"
#include "../libs/markdown.h"
#include "../libs/helper.h"

#define CMD_LEN 256


void print_log(log* doc_log){

    log* current_log = doc_log;
    while(current_log != NULL){
        printf("VERSION %ld\n", current_log->version_num);
        for(int i = 0; i < current_log->edits_num; i++){
            printf("EDIT %s %s %s", current_log->edits[i]->user, 
                current_log->edits[i]->command, 
                current_log->edits[i]->result);
            if(current_log->edits[i]->reject_reason != NULL){
                if(strlen(current_log->edits[i]->reject_reason) != 0 ){
                    printf(" %s", current_log->edits[i]->reject_reason);
                }
            }
            printf("\n");
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


// Get the log struct from the broadcast message.
log* get_log(char* message){
    log* new_log = init_log();
    // For strtok_r
    char* saveptr;
    // Get the copy message.
    char* msg_cpy = (char*)malloc(sizeof(char)*(strlen(message)+1));
    strcpy(msg_cpy, message);

    // Jump the version line.
    char* token = strtok(msg_cpy, "\n");

    if(token != NULL){
        // printf("Got version line: %s\n", token);
        char* ver_line = (char*)malloc(sizeof(char)*strlen(token) + 1);
        strcpy(ver_line, token);
        char* ver_save_ptr;
        char* word = strtok_r(token, " ", &ver_save_ptr);
        word = strtok_r(NULL, " ", &ver_save_ptr);
        size_t ver_num = strtol(word, NULL, 10);
        new_log->version_num = ver_num;

        free(ver_line);
    }

    token = strtok(NULL, "\n");

    // Handle every edit line.
    while(token != NULL){
        // Jump END line.
        // printf("Got line: %s\n", token);
        if(strcmp(token, "END") == 0){
            break;
        }
        // Copy the message, avoiding editing the original message.
        char* line = (char*)malloc(sizeof(char)*(strlen(token)+1));
        strcpy(line, token);
        
        // Four components for the edit.
        char user[CMD_LEN];
        char command[CMD_LEN];
        char result[CMD_LEN];
        char reject_reason[CMD_LEN] = "";

        
        char* word = strtok_r(line, " ", &saveptr);
        // printf("Word: %s|\n", word);
        if(word != NULL && strcmp(word, "EDIT") == 0) {

        // printf("Word: %s|\n", word);
            // Get username
            word = strtok_r(NULL, " ", &saveptr);
            if(word != NULL) {
                strcpy(user, word);
            }

            // Get the remaining part.
            char* remaining = strtok_r(NULL, "", &saveptr);
        // printf("Remaining: %s\n", remaining);
            if(remaining != NULL) {
                // Find the SUCCESS or Reject in the remaining.
                char* success_pos = strstr(remaining, " SUCCESS");
                char* reject_pos = strstr(remaining, " Reject");
                
                if(success_pos != NULL){
        // printf("Succ: %s\n", success_pos);
                }
                if(reject_pos != NULL){
        // printf("Rej: %s\n", reject_pos);
                }
                if(success_pos != NULL) {
                    *success_pos = '\0';
                    strcpy(command, remaining);
                    strcpy(result, "SUCCESS");
                } else if(reject_pos != NULL) {
                    *reject_pos = '\0';
                    strcpy(command, remaining);
                    strcpy(result, "Reject");
                    
                    // Get the reject reason
                    char* reason_part = reject_pos + 8;
                    if(strlen(reason_part) > 0) {
                        // printf("Reject reason: %s\n", reason_part);
                        strcpy(reject_reason, reason_part);
                    }
                }
            }
        }
        // printf("Parsed - User: %s, Command: %s, Result: %s", user, command, result);
        // if(strlen(reject_reason) > 0) {
        //     printf(", Reject reason: %s", reject_reason);
        // }
        // printf("\n");

        add_edit(&new_log, user, command, result, reject_reason);
        free(line);
        token = strtok(NULL, "\n");
    }
    // Free copied message.
    free(msg_cpy);
    return new_log;
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


// Get a copy of exist chunk list.
chunk* copy_chunk_list(chunk* chunk_head){
    chunk* new_chunk_list = (chunk*)malloc(sizeof(chunk));
    memcpy(new_chunk_list, chunk_head, sizeof(chunk));
    new_chunk_list->content = (char*)malloc(sizeof(char)*(strlen(chunk_head->content)+1));
    strcpy(new_chunk_list->content, chunk_head->content);

    chunk* temp = new_chunk_list;
    chunk* current_chunk = chunk_head->next_chunk;
    while(current_chunk != NULL){
        chunk* new_chunk = (chunk*)malloc(sizeof(chunk));
        memcpy(new_chunk, current_chunk, sizeof(chunk));
        new_chunk->content = (char*)malloc(sizeof(char)*(strlen(current_chunk->content)+1));
        strcpy(new_chunk->content, current_chunk->content);
        temp->next_chunk = new_chunk;

        current_chunk = current_chunk->next_chunk;
        temp = new_chunk;
    }

    return new_chunk_list;
}

void free_chunk_list(chunk* chunk_head){
    chunk* current = chunk_head;
    while(current != NULL){
        chunk* temp = current;
        current = current->next_chunk;

        free(temp->content);
        free(temp);
    }
}