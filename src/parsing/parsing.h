#ifndef __PARSING_H__
#define __PARSING_H__ 1
#include <stdlib.h>
#include "../tokens.h"
#include "../utils.h"

static LinkedList* processed = NULL;
static Token* program = NULL;

void parser_init(void) {
    processed = linkedlist_create((LinkedListEq)strcmp);
    program = (Token*)malloc(sizeof(Token));
    program->type = Node;
    // the 'program' token cannot have meaningful values for its source
    program->column = -1;
    program->line = -1;
    program->file = strmove("#INTERNAL#");
    program->data.node = linkedlist_create(linkedlist_pointereq);
}

Token* parse_file(char* fname) {
    if (linkedlist_indexof(processed, fname)) {
        return program;
    }
    todo();
    return program;
}

#endif
