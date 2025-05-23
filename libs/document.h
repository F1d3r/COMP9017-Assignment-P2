#ifndef DOCUMENT_H

#define DOCUMENT_H

#include <stdio.h>
#include <stdint.h>
/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible. 
 * Ensure you DO NOT change the name of document struct.
 */

typedef struct chunk{
    // TODO
    uint32_t offset;
    uint32_t length;
    char* content;
    struct chunk* next_chunk;

} chunk;

typedef struct document{
    // TODO
    uint64_t version_num;
    uint64_t doc_len;
    struct chunk* first_chunk;

    
} document;

// Functions from here onwards.
#endif
