#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libs/markdown.h"

#define SUCCESS 0 

// === Init and Free ===
document *markdown_init(void) {
    document* newDoc = (document*)malloc(sizeof(document));
    
    newDoc->version_num = 0;
    newDoc->doc_len = 0;

    newDoc->first_chunk = (chunk*)malloc(sizeof(chunk));
    newDoc->first_chunk->next_chunk = NULL;
    newDoc->first_chunk->offset = 0;
    newDoc->first_chunk->length = 0;
    newDoc->first_chunk->content = (char*)malloc(sizeof(char)*1);
    // newDoc->first_chunk->content[0] = '\n';
    newDoc->first_chunk->content[0] = '\0';
    // newDoc->first_chunk->content[strlen(newDoc->first_chunk->content)] = '\0';

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
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content) {
    (void)doc; (void)version; (void)pos; (void)content;
    return SUCCESS;
}

int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len) {
    (void)doc; (void)version; (void)pos; (void)len;
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
    char* flatten_content = malloc(sizeof(char)*doc->doc_len);
    flatten_content[0] = '\0';
    chunk* current_chunk = doc->first_chunk;
    while(current_chunk != NULL){

        strncpy(flatten_content+current_chunk->offset, 
            current_chunk->content, current_chunk->length);
        current_chunk = current_chunk->next_chunk;
    }
    return flatten_content;
}

// === Versioning ===
void markdown_increment_version(document *doc) {
    doc->version_num++;
}

