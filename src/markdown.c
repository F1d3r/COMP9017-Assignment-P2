#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/markdown.h"
#include "../libs/document.h"

#define SUCCESS 0 
#define INVALID_CURSOR_POS -1
#define DELETED_POSITION -2
#define OUTDATED_VERSION -3

// === Init and Free ===
document *markdown_init(void) {
    document* newDoc = (document*)malloc(sizeof(document));
    newDoc->version_num = 0;
    newDoc->doc_len = 0;

    newDoc->first_chunk = (chunk*)malloc(sizeof(chunk));
    newDoc->first_chunk->next_chunk = NULL;
    newDoc->first_chunk->offset = 0;
    newDoc->first_chunk->length = 0;
    newDoc->first_chunk->content = (char*)malloc(sizeof(char)*(newDoc->first_chunk->length+1));
    newDoc->first_chunk->content[newDoc->first_chunk->length] = '\0';


    // Next version.
    newDoc->next_version = (document*)malloc(sizeof(document));
    memcpy(newDoc->next_version, newDoc, sizeof(document));
    newDoc->next_version->first_chunk = copy_chunk_list(newDoc->first_chunk);

    return newDoc;
}

void markdown_free(document *doc) {
    free_chunk_list(doc->first_chunk);
    free_chunk_list(doc->next_version->first_chunk);
    free(doc->next_version);
    free(doc);
    return;
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    
    char* new_content = (char*)malloc(sizeof(char)*(doc->next_version->first_chunk->length+strlen(content)+1)); 
    memcpy(new_content, doc->next_version->first_chunk->content, pos);
    memcpy(new_content+pos, content, strlen(content));
    memcpy(new_content+pos+strlen(content), doc->next_version->first_chunk->content+pos, 
        doc->next_version->first_chunk->length-pos);
    free(doc->next_version->first_chunk->content);
    new_content[doc->next_version->first_chunk->length+strlen(content)] = '\0';

    doc->next_version->first_chunk->content = new_content;
    doc->next_version->first_chunk->length += strlen(content);
    doc->next_version->doc_len += strlen(content);

    doc->next_version->version_num = doc->version_num+1;
    return SUCCESS;
}

int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len) {
    if(version != doc->version_num){
        return OUTDATED_VERSION;
    }
    if(pos > doc->first_chunk->length){
        return INVALID_CURSOR_POS;
    }
    if(pos+len > doc->first_chunk->length){
        return INVALID_CURSOR_POS;
    }
    char* new_content = (char*)malloc(sizeof(char)*(strlen(doc->first_chunk->content)-len+1));
    strncpy(new_content, doc->next_version->first_chunk->content, pos);
    printf("Chunk length: %d\n", doc->next_version->first_chunk->length);
    if(pos+len < doc->next_version->first_chunk->length){
        strncpy(new_content+pos, doc->next_version->first_chunk->content+pos+len, 
            strlen(doc->next_version->first_chunk->content)-pos-len+1);
    }
    new_content[doc->next_version->first_chunk->length - len] = '\0';
    char* temp = doc->next_version->first_chunk->content;
    doc->next_version->first_chunk->content = new_content;
    free(temp);
    doc->next_version->first_chunk->length -= len;
    doc->next_version->doc_len -= len;

    doc->next_version->version_num = doc->version_num+1;
    return SUCCESS;
}


// === Formatting Commands ===
int markdown_newline(document *doc, uint64_t version, size_t pos) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->doc_len);
        return INVALID_CURSOR_POS;
    }
    char* newline_symbol = (char*)malloc(sizeof(char)*2);
    strcpy(newline_symbol, "\n");
    markdown_insert(doc, version, pos, newline_symbol);
    free(newline_symbol);
    
    return SUCCESS;
}

int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos) {
    if(version != doc->version_num){
        printf("Version outdated: %ld|%ld\n", version, doc->version_num);
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        printf("Invalid position: %ld|%ld\n", pos, doc->doc_len);
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
    strcpy(newline_symbol, "\n---\n");
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
    char* flatten_content = (char*)malloc(sizeof(char));
    chunk* current_chunk = doc->first_chunk;
    int size = 0;
    while(current_chunk != NULL){
        size += current_chunk->length;
        flatten_content = realloc(flatten_content, sizeof(char)*(size+1));
        // Problem here.
        memcpy(flatten_content+current_chunk->offset, 
            current_chunk->content, sizeof(char)*current_chunk->length);
        current_chunk = current_chunk->next_chunk;
    }
    flatten_content[doc->doc_len] = '\0';
    return flatten_content;
}

// === Versioning ===
void markdown_increment_version(document* doc) {
    chunk* temp = doc->first_chunk;
    memcpy(doc, doc->next_version, sizeof(document));
    doc->next_version->first_chunk = copy_chunk_list(doc->first_chunk);

    free_chunk_list(temp);

}

