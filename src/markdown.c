#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/markdown.h"
#include "../libs/document.h"
#include "../libs/helper.h"

#define SUCCESS 0 
#define INVALID_CURSOR_POS -1
#define DELETED_POSITION -2
#define OUTDATED_VERSION -3

#define CMD_LEN 256

// === Init and Free ===
document *markdown_init(void) {
    document* newDoc = (document*)malloc(sizeof(document));
    newDoc->version_num = 0;
    newDoc->doc_len = 0;

    // Initialize the log.
    newDoc->log_head = init_log();

    // Initialize the first chunk.
    newDoc->first_chunk = (chunk*)malloc(sizeof(chunk));
    newDoc->first_chunk->next_chunk = NULL;
    newDoc->first_chunk->offset = 0;
    newDoc->first_chunk->length = 0;
    newDoc->first_chunk->content = 
        (char*)malloc(sizeof(char)*(newDoc->first_chunk->length+1));
    newDoc->first_chunk->content[newDoc->first_chunk->length] = '\0';

    // Initialize the next doc.
    newDoc->next_doc = (document*)malloc(sizeof(document));
    newDoc->next_doc->version_num = 0;
    newDoc->next_doc->doc_len = 0;
    newDoc->next_doc->first_chunk = (chunk*)malloc(sizeof(chunk));
    newDoc->next_doc->first_chunk->next_chunk = NULL;
    newDoc->next_doc->first_chunk->offset = 0;
    newDoc->next_doc->first_chunk->length = 0;
    newDoc->next_doc->first_chunk->content = 
        (char*)malloc(sizeof(char)*(newDoc->next_doc->first_chunk->length+1));
    newDoc->next_doc->first_chunk->content[
        newDoc->next_doc->first_chunk->length] = '\0';
    newDoc->next_doc->next_doc = NULL;


    return newDoc;
}

void markdown_free(document *doc) {
    log_free(doc->log_head);
    free_chunk_list(doc->first_chunk);
    free_chunk_list(doc->next_doc->first_chunk);
    free(doc->next_doc);
    free(doc);
    return;
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content) {
    document* next_doc = doc->next_doc;
    
    if(version != next_doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, next_doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > next_doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, next_doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    
    char* new_content = (char*)malloc(sizeof(char)*(next_doc->first_chunk->length+strlen(content)+1)); 
    memcpy(new_content, next_doc->first_chunk->content, pos);
    memcpy(new_content+pos, content, strlen(content));
    memcpy(new_content+pos+strlen(content), next_doc->first_chunk->content+pos, 
        next_doc->first_chunk->length-pos);
    new_content[next_doc->first_chunk->length+strlen(content)] = '\0';
    free(next_doc->first_chunk->content);

    next_doc->first_chunk->content = new_content;
    next_doc->first_chunk->length += strlen(content);
    next_doc->doc_len += strlen(content);

    return SUCCESS;
}

int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len) {
    document* next_doc = doc->next_doc;
    
    if(version != next_doc->version_num){
        return OUTDATED_VERSION;
    }
    if(pos > next_doc->first_chunk->length){
        return INVALID_CURSOR_POS;
    }
    if(pos+len > next_doc->first_chunk->length){
        len = next_doc->first_chunk->length - pos;
    }
    char* new_content = (char*)malloc(sizeof(char)*(strlen(next_doc->first_chunk->content)-len+1));
    strncpy(new_content, next_doc->first_chunk->content, pos);
    printf("Chunk length: %d\n", next_doc->first_chunk->length);
    if(pos+len < next_doc->first_chunk->length){
        strncpy(new_content+pos, next_doc->first_chunk->content+pos+len, 
            strlen(next_doc->first_chunk->content)-pos-len+1);
    }
    new_content[next_doc->first_chunk->length - len] = '\0';
    char* temp = next_doc->first_chunk->content;
    next_doc->first_chunk->content = new_content;
    free(temp);
    next_doc->first_chunk->length -= len;
    next_doc->doc_len -= len;

    return SUCCESS;
}


// === Formatting Commands ===
int markdown_newline(document *doc, uint64_t version, size_t pos) {
    if(version != doc->next_doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->next_doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->next_doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->next_doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    char* newline_symbol = (char*)malloc(sizeof(char)*2);
    strcpy(newline_symbol, "\n");
    markdown_insert(doc, version, pos, newline_symbol);
    free(newline_symbol);
    
    return SUCCESS;
}

int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos) {
    if(version != doc->next_doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->next_doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->next_doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->next_doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    char* heading_symbol = (char*)malloc(sizeof(char)*5);
    for(int i = 0; i < level; i++){
        strcpy(heading_symbol+i, "#");
    }
    strcpy(heading_symbol+strlen(heading_symbol), " ");
    markdown_insert(doc, version, pos, heading_symbol);
    free(heading_symbol);
    
    return SUCCESS;
}

int markdown_bold(document *doc, uint64_t version, size_t start, size_t end) {
    if(version != doc->next_doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->next_doc->version_num);
        return OUTDATED_VERSION;
    }
    if(start > doc->next_doc->doc_len){
        printf("Invalid position: %ld|%ld\n", start, doc->next_doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(end > doc->next_doc->doc_len+1){
        printf("Invalid position: %ld|%ld\n", start, doc->next_doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(start > end){
        printf("Invalid position: %ld|%ld\n", start, end);
        return INVALID_CURSOR_POS;
    }
    char* bold_symbol = (char*)malloc(sizeof(char)*3);
    strcpy(bold_symbol, "**");
    markdown_insert(doc, version, end, bold_symbol);
    markdown_insert(doc, version, start, bold_symbol);
    free(bold_symbol);
    
    return SUCCESS;
}

int markdown_italic(document *doc, uint64_t version, size_t start, size_t end) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(start > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(end > doc->doc_len+1){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(start > end){
        printf("Invalid position: %ld|%ld\n", start, end);
        return INVALID_CURSOR_POS;
    }
    char* bold_symbol = (char*)malloc(sizeof(char)*3);
    strcpy(bold_symbol, "*");
    markdown_insert(doc, version, end, bold_symbol);
    markdown_insert(doc, version, start, bold_symbol);
    free(bold_symbol);
    
    return SUCCESS;
}

int markdown_blockquote(document *doc, uint64_t version, size_t pos) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    char* newline_symbol = (char*)malloc(sizeof(char)*3);
    strcpy(newline_symbol, "> ");
    markdown_insert(doc, version, pos, newline_symbol);
    free(newline_symbol);
    
    return SUCCESS;
}

int markdown_ordered_list(document *doc, uint64_t version, size_t pos) {
    (void)doc; (void)version; (void)pos;
    return SUCCESS;
}

int markdown_unordered_list(document *doc, uint64_t version, size_t pos) {
    (void)doc; (void)version; (void)pos;
    return SUCCESS;
}

int markdown_code(document *doc, uint64_t version, size_t start, size_t end) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(start > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(end > doc->doc_len+1){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(start > end){
        printf("Invalid position: %ld|%ld\n", start, end);
        return INVALID_CURSOR_POS;
    }
    char* bold_symbol = (char*)malloc(sizeof(char)*2);
    strcpy(bold_symbol, "`");
    markdown_insert(doc, version, end, bold_symbol);
    markdown_insert(doc, version, start, bold_symbol);
    free(bold_symbol);
    
    return SUCCESS;
}

int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    char* newline_symbol = (char*)malloc(sizeof(char)*6);
    strcpy(newline_symbol, "---\n");
    markdown_insert(doc, version, pos, newline_symbol);
    free(newline_symbol);
    
    return SUCCESS;
}

int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(start > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(end > doc->doc_len+1){
        printf("Invalid position: %ld|%ld\n", start, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    if(start >= end){
        printf("Invalid position: %ld|%ld\n", start, end);
        return INVALID_CURSOR_POS;
    }
    char* start_symbol = (char*)malloc(sizeof(char)*2);
    strcpy(start_symbol, "[");
    char* end_symbol = (char*)malloc(sizeof(char)*(strlen(url)+4));
    strcpy(end_symbol, "](");
    strcpy(end_symbol+2, url);
    strcpy(end_symbol+2+strlen(url), ")");
    markdown_insert(doc, version, end, end_symbol);
    markdown_insert(doc, version, start, start_symbol);
    free(start_symbol);
    free(end_symbol);
    
    return SUCCESS;
}

// === Utilities ===
void markdown_print(const document *doc, FILE *stream) {
    char* doc_content = markdown_flatten(doc);
    if (doc_content != NULL) {
        fprintf(stream, "%s\n", doc_content);
        fflush(stream);
    }
    free(doc_content);
}

char *markdown_flatten(const document *doc) {
    char* flatten_content = malloc(sizeof(char)*(doc->doc_len+1));
    memset(flatten_content, 0, doc->doc_len + 1);
    chunk* current_chunk = doc->first_chunk;
    while(current_chunk != NULL){
        memcpy(flatten_content+current_chunk->offset, 
            current_chunk->content, sizeof(char)*current_chunk->length);
        current_chunk = current_chunk->next_chunk;
    }
    flatten_content[doc->doc_len] = '\0';
    return flatten_content;
}

// === Versioning ===
// Commit the change in the next document.
void markdown_increment_version(document* doc) {
    // Go the the latest log.
    log* last_log = doc->log_head;
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }

    // Check any updated made.
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }
    for(int i = 0; i < last_log->edits_num; i++){
        if(strcmp(last_log->edits[i]->result, "SUCCESS") == 0){
            // Copy the change if any.
            free_chunk_list(doc->first_chunk);
            doc->first_chunk = copy_chunk_list(doc->next_doc->first_chunk);
            doc->next_doc->version_num += 1;
            break;
        }
    }
    doc->version_num = doc->next_doc->version_num;
    doc->doc_len = doc->next_doc->doc_len;

}


void update_doc(document* doc){
    // Go the the latest log.
    log* last_log = doc->log_head;
    while(last_log->next_log != NULL){
        last_log = last_log->next_log;
    }

    // Iterate through the latest log. Process every command.
    for(int i = 0; i < last_log->edits_num; i ++){
        // Only process success commands.
        if(strcmp(last_log->edits[i]->result, "SUCCESS") == 0){
            char command_input[CMD_LEN];
            strcpy(command_input, last_log->edits[i]->command);
            char* command = NULL;
            char* arg1 = NULL;
            char* arg2 = NULL;
            char* arg3 = NULL;
            resolve_command(command_input, &command, &arg1, &arg2, &arg3);

            if(strcmp(command, "INSERT") == 0){
                printf("Inserting to document.\n");
                size_t pos = strtol(arg1, NULL, 10);
                markdown_insert(doc, last_log->version_num, pos, arg2);
            }
            else if(strcmp(command, "DEL") == 0){
                printf("Deleting from document.\n");
                size_t pos = strtol(arg1, NULL, 10);
                size_t len = strtol(arg2, NULL, 10);
                markdown_delete(doc, last_log->version_num, pos, len);
            }
            else if(strcmp(command, "BOLD") == 0){
                printf("Bolding document.\n");
                size_t pos1 = strtol(arg1, NULL, 10);
                size_t pos2 = strtol(arg2, NULL, 10);
                markdown_bold(doc, last_log->version_num, pos1, pos2);
            }
            else if(strcmp(command, "NEWLINE") == 0){
                printf("Newline document.\n");
                size_t pos = strtol(arg1, NULL, 10);
                markdown_newline(doc, last_log->version_num, pos);
            }
            else if(strcmp(command, "HEADING") == 0){
                printf("Newline document.\n");
                size_t level = strtol(arg1, NULL, 10);
                size_t pos = strtol(arg2, NULL, 10);
                markdown_heading(doc, last_log->version_num, level, pos);
            }
            else if(strcmp(command, "ITALIC") == 0){
                printf("Italicing document.\n");
                size_t start = strtol(arg1, NULL, 10);
                size_t end = strtol(arg2, NULL, 10);
                markdown_italic(doc, last_log->version_num, start, end);
            }
            else if(strcmp(command, "BLOCKQUOTE") == 0){
                printf("Blocking quote document.\n");
                size_t pos = strtol(arg1, NULL, 10);
                markdown_blockquote(doc, last_log->version_num, pos);
            }
            else if(strcmp(command, "CODE") == 0){
                printf("Coding document.\n");
                size_t start = strtol(arg1, NULL, 10);
                size_t end = strtol(arg2, NULL, 10);
                markdown_code(doc, last_log->version_num, start, end);
            }
            else if(strcmp(command, "HORIZONTAL_RULE") == 0){
                printf("Ruling document.\n");
                size_t pos = strtol(arg1, NULL, 10);
                markdown_horizontal_rule(doc, last_log->version_num, pos);
            }
            else if(strcmp(command, "LINK") == 0){
                printf("Linking document.\n");
                size_t start = strtol(arg1, NULL, 10);
                size_t end = strtol(arg2, NULL, 10);
                markdown_link(doc, last_log->version_num, start, end, arg3);
            }
        }
    }
}
