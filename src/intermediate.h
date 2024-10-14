#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

#include "utils.h"

typedef enum InstructionType {
    Goto,   // Goto(INSTR)        : go to instruction INSTR
    Decl,   // Decl(VAR, TYPE)    : declare variable VAR of TYPE
    Store,  // Store(VAR, EXPR)   : evaluate EXPR and store into VAR
    Call,   // Call(EXPR)         : evaluate EXPR
    Return, // Return(EXPR)       : evaluate and return EXPR
    Branch  // Branch(COND, L, R) : evaluate COND and go to L if true, otherwise go to R
} InstructionType;

typedef struct Instruction {
  InstructionType type; // the type of instruction
  int i;                // contains first integer, if necessary
  int j;                // contains second integer, if necessary
  Expression* expr;     // contains expression, if necessary
} Instruction;

// this gives easy-to-use constants for certain built-ins
#define VAR = 1;
#define ADD = 2;
#define SUB = 3;
#define MUL = 4;
#define DIV = 5;
#define POW = 6;
// TODO: More arithmetic operators

typedef struct Expression {
  int name;         // the number id of the 'function'
  LinkedList* args; // the arguments of the 'function'
} Expression;

typedef struct Function {
  LinkedList* arg_types;    // the types of the arguments
  LinkedList* instructions; // the list of instructions in the function
} Function;

typedef struct Program {
  LinkedList* types;
  LinkedList* functions;
} Program;

Program compile(Token t) {
    
}

Function compile_function(Token t) {
    
}
#endif
