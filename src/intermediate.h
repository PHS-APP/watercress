#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

typedef enum InstructionType {
    Goto,   // Goto(INSTR)        : go to instruction INSTR
    Decl,   // Decl(Var)          : declare variable VAR
    Store,  // Store(VAR, EXPR)   : evaluate EXPR and store into VAR
    Call,   // Call(EXPR)         : evaluate EXPR
    Return, // Return(EXPR)       : evaluate and return EXPR
    Branch  // Branch(COND, L, R) : evaluate COND and go to L if true, otherwise go to R
} InstructionType;

typedef struct Instruction {
  InstructionType type; // the type of instruction
  int i;                // contains first integer (var or instr), if necessary
  int j;                // contains second integer (var or instr), if necessary
  Expression* expr;     // contains expression, if necessary
} Instruction;

typedef enum ExpressionType {
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    // TODO: More arithmetic operators
    Var,
    FunCall
} ExpressionType;

typedef struct Expression {
  ExpressionType type;
  LinkedList* args;
} Expression;
#endif
