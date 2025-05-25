#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/markdown.h"

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

    // strcpy(newDoc->first_chunk->content, "1234");
    newDoc->first_chunk->content[newDoc->first_chunk->length] = '\0';

    return newDoc;
}

void markdown_free(document *doc) {
    chunk* current_chunk = doc->first_chunk;
    while(current_chunk != NULL){
        chunk* temp = current_chunk->next_chunk;
        free(current_chunk->content);
        free(current_chunk);
        current_chunk = temp;
    }

    free(doc);
    return;
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, char *content) {
    if(version != doc->version_num){
        return OUTDATED_VERSION;
    }
    if(pos > doc->doc_len){
        return INVALID_CURSOR_POS;
    }
    
    char* new_content = (char*)malloc(sizeof(char)*(doc->first_chunk->length+strlen(content)+1)); 
    memcpy(new_content, doc->first_chunk->content, pos);
    memcpy(new_content+pos, content, strlen(content));
    memcpy(new_content+pos+strlen(content), doc->first_chunk->content+pos, doc->first_chunk->length-pos);
    free(doc->first_chunk->content);
    new_content[doc->first_chunk->length+strlen(content)] = '\0';

    doc->first_chunk->content = new_content;
    doc->first_chunk->length += strlen(content);
    doc->doc_len += strlen(content);


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
    strncpy(new_content, doc->first_chunk->content, pos);
    if(pos+len < doc->first_chunk->length){
        strncpy(new_content+pos, doc->first_chunk->content+pos+len, 
            strlen(doc->first_chunk->content)-pos-len+1);
    }
    char* temp = doc->first_chunk->content;
    doc->first_chunk->content = new_content;
    free(temp);
    doc->first_chunk->length -= len;
    doc->doc_len -= len;

    return SUCCESS;
}


// === Formatting Commands ===
int markdown_newline(document *doc, uint64_t version, size_t pos) {
    (void)doc; (void)version; (void)pos;
    return SUCCESS;
}

int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos) {
    (void)doc; (void)version; (void)level; (void)pos;
    return SUCCESS;
}

int markdown_bold(document *doc, uint64_t version, size_t start, size_t end) {
    (void)doc; (void)version; (void)start; (void)end;
    return SUCCESS;
}

int markdown_italic(document *doc, uint64_t version, size_t start, size_t end) {
    (void)doc; (void)version; (void)start; (void)end;
    return SUCCESS;
}

int markdown_blockquote(document *doc, uint64_t version, size_t pos) {
    (void)doc; (void)version; (void)pos;
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
    (void)doc; (void)version; (void)start; (void)end;
    return SUCCESS;
}

int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos) {
    (void)doc; (void)version; (void)pos;
    return SUCCESS;
}

int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url) {
    (void)doc; (void)version; (void)start; (void)end; (void)url;
    return SUCCESS;
}

// === Utilities ===
void markdown_print(const document *doc, FILE *stream) {
    char* doc_content = markdown_flatten(doc);
    if (doc_content != NULL) {
        fprintf(stream, "%s", doc_content);
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
void markdown_increment_version(document *doc) {
    doc->version_num++;
}

