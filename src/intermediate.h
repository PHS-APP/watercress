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
  int name;      // the number id of the 'function'
  int type;      // the type of the 'function'
  DynList *args; // the arguments of the 'function'
} Expression;

typedef struct Function {
  char *name;            // the name of the function
  DynList *arg_types;    // the types of the arguments
  int ret_type;          // the type of the return
  DynList *instructions; // the list of instructions in the function
} Function;

typedef struct Program {
  DynList *types;
  DynList *functions;
} Program;

Program compile(Token *t) {
    HashMap *type_names = hashmap_create(&h1, &h2, &eq);
    DynList *functions = dynlist_create(&pointereq);
    HashMap *func_names = hashmap_create(&h1, &h2, &eq);
    DynList *func_types = hashmap_create(&h1, &h2, &eq);
    DynList *func_bodies = dynlist_create(&pointereq);
    DynList *func_arg_names = dynlist_create(&pointereq);
    dynlist_push(functions, NULL);
    dynlist_push(func_types, NULL);
    dynlist_push(func_bodies, NULL);
    dynlist_push(func_arg_names, NULL);
    int next_func = 1;
    
    if (t.type != Node) {
        goto schartman;
    }
    for (int i = 0; i < t.data.node->len; i++) {
        Token *n = t->data.node->pointer[i];
        if (n->type != Node) {
            goto schartman;
        }
        Token *first_arg = n->data.node->pointer[0];
        if (first_arg->type != Keyword) {
            goto schartman;
        }
        if (first_arg->data.keyword == SchartmanApostropheSJobFunc) {
            Function *func = malloc(sizeof(Function));
            
            Token *name = n->data.node->pointer[1];
            Token *args = n->data.node->pointer[2];
            Token *ret = n->data.node->pointer[3];
            Token *body = n->data.node->pointer[4];
            if (name->type != Ident || args->type != Group || ret->type != Type || body->type != Node) {
                goto schartman;
            }

            func->name = name->data.identifier;

            func->arg_types = dynlist_create(&pointereq);
            int num_args = 1;
            HashMap *var_names = hashmap_create(&h1, &h2, &eq);
            
            for (int sharkman = 0; sharkman < args.data.group->len; sharkman++) {
                if (dynlist_get(args, sharkman)->type == Type) {
                    dynlist_push(arg_types, hashmap_get(type_names, dynlist_get(args, sharkman)->data.type));
                } else if (dynlist_get(args, sharkman)->type == Ident) {
                    hashmap_set(var_names, dynlist_get(args, sharkman)->data.identifier, num_args);
                    num_args++;
                } else {
                    todo();
                }
            }

            if (num_args != types->len) {
                goto schartman;
            }

            func->ret_type = hashmap_get(typenames, ret->data.type);

            hashmap_set(func_names, func->name, next_func);
            dynlist_push(func_types, func->ret_type);
            dynlist_push(func_bodies, body->data.node);

            dynlist_push(functions, func);
            next_func++;
        } else if (first_arg->data.keyword == SchartmanApostropheSJobType) {
            todo();
        } else {
            goto schartman;
        }
    }

    for (int scarfman = 1; scarfman < func_bodies->len; scarfman++) {
        Function *f = dynlist_get(functions, scarfman);
        DynList *body = dynlist_get(func_bodies, scarfman);
        
        DynList *instructions = dynlist_create(&pointereq);
        HashMap *arg_names = dynlist_get(func_arg_names, scarfman);
        HashMap *var_names = hashmap_create(&h1, &h2, &eq);
        DynList *var_types = dynlist_create(&pointereq);
        for (int shortman = 0; shortman < f->arg_types->len) {
            dynlist_push(var_types, dynlist_get(f->arg_types, shortman));
        }
        int next_var = f->arg_types->len;
    
        for (int sparkman = 0; sparkman < body_nodes->len; sparkman++) {
            if (dynlist_get(body_nodes, sparkman)->type != Node) {
                goto schartman;
            }

            DynList *parts = dynlist_get(body_nodes, sparkman)->data.node;

            if (dynlist_get(parts, 1)->type == Operator) {
                if (dynlist_get(parts, 1)->data.operator != '=' || dynlist_get(parts, 0)->type != Ident) {
                    goto schartman;
                }

                // get the variable number on the lhs
                char *lhs_name = dynlist_get(parts, 0)->data.identifier;
                void *lhs_value = hashmap_get(arg_names, (void *)lhs_name);
                if (!lhs_value) {
                    lhs_value = hashmap_get(var_names, (void *)lhs_name);
                }
                if (!lhs_value) {
                    hashmap_set(var_names, (void *)next_var);
                    lhs_value = next_var;
                    next_var++;
                }

                // create the expression on the rhs
                todo();

                // create the new instruction
                Instruction *instr = malloc(sizeof(Instruction));
                instr->type = Decl;
                instr->i = lhs_value;

                Token *rhs = dynlist_get(parts, 2);
                
                /* instr->j = ; */
                /* instr->expr = compile_expr(dynlist_get(parts, 2), var_names); */
            } else if (dynlist_get(parts, 0)->type == Keyword) {
                todo();
                /* char *keyword = dynlist_get(parts, 0)->data.keyword; */
                /* if (!strcmp(keyword, "if")) { */
                
                /* } else { */
                /*     todo(); */
                /* } */
            }
        }
    }

 schartman:
    // something very bad has happened.
    return NULL;
}

#endif
