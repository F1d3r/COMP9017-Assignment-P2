#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible. 
 * Ensure you DO NOT change the name of document struct.
 */

typedef struct chunk{
    uint32_t offset;
    uint32_t length;
    char* content;
    struct chunk* next_chunk;

} chunk;

typedef struct document{
    uint64_t version_num;
    uint64_t doc_len;
    struct chunk* first_chunk;
    struct log* log_head;
} document;

typedef struct edit{
    char* user;
    char* command;
    char* result;
    char* reject_reason;
} edit;

typedef struct log{
    uint64_t version_num;
    edit** edits;   // edit* array
    int edits_num;
    struct log* next_log;
}log;

void print_log(log* doc_log);

void add_log(log** doc_log, log* new_log);

void add_edit(log** log_head, char* user, char* command, char* result, char* reject_reason);

log* init_log();

log* get_log(char* message);

void log_free(log* doc_log);

chunk* copy_chunk_list(chunk* chunk_head);

void free_chunk_list(chunk* chunk_head);

// Functions from here onwards.

#endif
