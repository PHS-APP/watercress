#ifndef __TOKENS_H__
#define __TOKENS_H__ 1
#include "./types.h"
#include "./utils.h"

typedef enum TokenType {
    Node, // groups tokens together to make interacting with the AST easier
    Ident, // identifier
    Bool, // boolean
    Int, // integer
    Float, // floating point
    Keyword,
    Operator,
    Char, // character
    String,
    Group, // group of other tokens
    Stmt, // contains a 'line'
    Sep, // separators for parameters and lists
    Type,
    Mod, // ident modifiers
    // all tokens below this point cannot be generated by the parser
    Asm, // pseudo-assembly generated through optimizing phase
    Meta // metadata that should be passed to the assembly generation phase
} TokenType;

typedef struct AsmToken {
    /* todo */
} AsmToken;

typedef union TokenData {
    char *string, *identifier, *type;
    char character, boolean;
    ushort keyword, operator, modifier;
    long integer;
    double floating;
    LinkedList *group, *statement, *node;
    void* meta;
    AsmToken* assembly;
} TokenData;

typedef struct Token {
    TokenType type;
    long line, column;
    char* file;
    TokenData data;
} Token;

/*
funcdef: Node(Keyword(func), Ident([name]), Group[Type([type1]), Ident([param1]), ...], Type([return type]), Node([body]))
typedef: Node(Keyword(type), Ident([name]), Group(Type([type1]), Ident(member1), ...))
ifstmts: Node(Keyword(if), Group([condition]), Node([body]), [Keyword(else), Node([body])]?)
*/

#endif
