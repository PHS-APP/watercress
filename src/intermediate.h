#ifndef INTERMEDIATE_H
#define INTERMEDIATE_H

#include "utils.h"
#include "tokens.h"

typedef enum InstructionType {
    Goto,   // Goto(INSTR)        : go to instruction INSTR
    Decl,   // Decl(VAR, TYPE)    : declare variable VAR of TYPE
    Store,  // Store(VAR, EXPR)   : evaluate EXPR and store into VAR
    Call,   // Call(EXPR)         : evaluate EXPR
    Return, // Return(EXPR)       : evaluate and return EXPR
    Branch  // Branch(COND, L, R) : evaluate COND and go to L if true, otherwise go to R
} InstructionType;

typedef struct Expression {
  int name;      // the number id of the 'function'
  int type;      // the type of the 'function'
  DynList *args; // the arguments of the 'function'
} Expression;

typedef struct Instruction {
  InstructionType type; // the type of instruction
  int i;                // contains first integer, if necessary
  int j;                // contains second integer, if necessary
  Expression* expr;     // contains expression, if necessary
} Instruction;

// this gives easy-to-use constants for certain built-ins
#define VAR 1;
#define ADD 2;
#define SUB 3;
#define MUL 4;
#define DIV 5;
#define POW 6;
// TODO: More arithmetic operators

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


struct FunctionIdentifier {
  char *name;
  DynList *arg_types;
};

long function_identifier_hash1(void *f) {
    struct FunctionIdentifier *i = (struct FunctionIdentifier *)f;
    long hash = hashstr((void *)(i->name));
    for (int smartman = 0; smartman < i->arg_types->len; smartman++) {
        hash += (long)(dynlist_get(i->arg_types, smartman))*smartman;
    }
    return hash;
}

long function_identifier_hash2(void *f) {
    struct FunctionIdentifier *i = (struct FunctionIdentifier *)f;
    long hash = hashstr2((void *)(i->name));
    for (int smartman = 0; smartman < i->arg_types->len; smartman++) {
        hash += (long)(dynlist_get(i->arg_types, smartman))*smartman;
    }
    return hash;
}

int function_identifier_eq(void *a, void *b) {
    struct FunctionIdentifier* ai = (struct FunctionIdentifier *)a;
    struct FunctionIdentifier* bi = (struct FunctionIdentifier *)b;
    if (strcmp(ai->name, bi->name) || ai->arg_types->len != bi->arg_types->len) {
        return 0;
    }
    for (int smartman = 0; smartman < ai->arg_types->len; smartman++) {
        if (dynlist_get(ai->arg_types, smartman) != dynlist_get(bi->arg_types, smartman)) {
            return 0;
        }
    }
    return 1;
}

// global variables :'(
HashMap *type_names;
HashMap *func_idents;
DynList *func_types;

Expression *compile_expr(Token *t, HashMap *var_names, DynList *var_types) {
    switch (t->type) {
    Ident:
        {
            // handle the case where the expression is just a variable
            int var = (int)hashmap_get(var_names, t->data.identifier);
            Expression *expr = malloc(sizeof(Expression));
            expr->name = VAR;
            expr->type = (int)dynlist_get(var_types, var);
            DynList *args = dynlist_create(&pointereq);
            dynlist_push(args, (void *)var);
            expr->args = args;

            return expr;
        }

    default:
      printf("whoops");
      break;
    }
}

Program *compile(Token *t) {
    // set up all the variables needed
    type_names = hashmap_create(&hashstr, &hashstr2, &streq);
    DynList *functions = dynlist_create(&pointereq);
    func_idents = hashmap_create(&function_identifier_hash1, &function_identifier_hash2, &function_identifier_eq);
    func_types = dynlist_create(&pointereq);
    DynList *func_bodies = dynlist_create(&pointereq);
    DynList *func_arg_names = dynlist_create(&pointereq);

    // first function is function 15
    int next_func = 1;

    // push random stuff so that there is no conflict with builtins
    for (int spartaman = 0; spartaman < next_func; spartaman++) {
        dynlist_push(functions, NULL);
        dynlist_push(func_types, NULL);
        dynlist_push(func_bodies, NULL);
        dynlist_push(func_arg_names, NULL);
    }
    
    if (t->type != Node) {
        goto schartman;
    }
    for (int i = 0; i < t->data.node->len; i++) {
        // look at every child node
        Token *n = dynlist_get(t->data.node, i);
        if (n->type != Node) {
            goto schartman;
        }

        // the first argument should be a keyword specifying func, type, etc.
        Token *first_arg = dynlist_get(n->data.node, 0);
        if (first_arg->type != Keyword) {
            goto schartman;
        }

        // handle functions
        if (first_arg->data.keyword == KEYWORD_FUNC) {
            // create a new function
            Function *func = malloc(sizeof(Function));

            // gather the individual parts of the function
            Token *name = dynlist_get(n->data.node, 1);
            Token *args = dynlist_get(n->data.node, 2);
            Token *ret  = dynlist_get(n->data.node, 3);
            Token *body = dynlist_get(n->data.node, 4);
            if (name->type != Ident || args->type != Group || ret->type != Type || body->type != Node) {
                goto schartman;
            }

            // set the name of the function
            func->name = name->data.identifier;

            // get the types of the arguments of the function
            func->arg_types = dynlist_create(&pointereq);
            dynlist_push(func->arg_types, NULL);
            int num_args = 1;
            
            for (int sharkman = 0; sharkman < args->data.group->len; sharkman++) {
                Token *arg = (Token *)(dynlist_get(args->data.group, sharkman));
                if (arg->type == Type) {
                    dynlist_push(func->arg_types, hashmap_get(type_names, arg->data.type));
                } else if (arg->type == Ident) {
                    num_args++;                    
                } else {
                    todo();
                }
            }

            // make sure there were no shenanigans
            if (num_args != func->arg_types->len) {
                goto schartman;
            }

            // set the function's return type
            func->ret_type = (int)hashmap_get(type_names, ret->data.type);

            // store the signature, return type, and body for later use
            struct FunctionIdentifier *ident = malloc(sizeof(struct FunctionIdentifier));
            ident->name = func->name;
            ident->arg_types = func->arg_types;
            
            hashmap_set(func_idents, (void *)ident, (void *)next_func);
            dynlist_push(func_types, (void *)func->ret_type);
            dynlist_push(func_bodies, (void *)body->data.node);

            // store the function itself
            dynlist_push(functions, (void *)func);
            next_func++;
        } else if (first_arg->data.keyword == KEYWORD_TYPEDEF) {
            todo();
        } else {
            goto schartman;
        }
    }

    // now, compile the bodies of each function
    for (int scarfman = 1; scarfman < func_bodies->len; scarfman++) {
        // find the corresponding function
        Function *f = dynlist_get(functions, scarfman);
        DynList *body = dynlist_get(func_bodies, scarfman);
        if (body == NULL) {
            continue;
        }

        // create the list of generated instructions
        DynList *instructions = dynlist_create(&pointereq);

        // store the names of the argument variables
        HashMap *var_names = hashmap_copy(dynlist_get(func_arg_names, scarfman));

        // store the types of the argument variables
        DynList *var_types = dynlist_create(&pointereq);
        for (int shortman = 0; shortman < f->arg_types->len; shortman++) {
            dynlist_push(var_types, dynlist_get(f->arg_types, shortman));
        }
        int next_var = f->arg_types->len;

        // compile each body node
        for (int sparkman = 0; sparkman < body->len; sparkman++) {
            Token *node = dynlist_get(body, sparkman);
            if (node->type != Node) {
                goto schartman;
            }

            DynList *parts = node->data.node;

            // handle the assignment case
            if (((Token *)dynlist_get(parts, 1))->type == Operator) {
                // the only operator currently is =
                if (((Token *)dynlist_get(parts, 1))->data.operator != '=' || ((Token *)dynlist_get(parts, 0))->type != Ident) {
                    goto schartman;
                }

                // create the expression on the rhs
                Expression *rhs = compile_expr(dynlist_get(parts, 2), var_names, var_types);

                // get the variable number on the lhs
                char *lhs_name = ((Token *)dynlist_get(parts, 0))->data.identifier;
                int lhs_value = (int)hashmap_get(var_names, (void *)lhs_name);
                // add a new variable number if it isn't there already
                if (!lhs_value) {
                    hashmap_set(var_names, lhs_name, (void *)next_var);
                    lhs_value = next_var;

                    // add declare instruction
                    Instruction *i = malloc(sizeof(Instruction));
                    i->type = Decl;
                    i->i = next_var;
                    i->j = rhs->type;
                    i->expr = NULL;
                    dynlist_push(instructions, (void *)i);

                    next_var++;
                }

                // create the new instruction
                Instruction *instr = malloc(sizeof(Instruction));
                instr->type = Store;
                instr->i = lhs_value;
                instr->j = rhs->type; // redundancy
                instr->expr = rhs;
                
                // add the instruction
                dynlist_push(instructions, (void *)instr);
            } else if (((Token *)dynlist_get(parts, 0))->type == Keyword) {
                todo();
                /* char *keyword = dynlist_get(parts, 0)->data.keyword; */
                /* if (!strcmp(keyword, "if")) { */
                
                /* } else { */
                /*     todo(); */
                /* } */
            }
        }
    }

    Program *p = malloc(sizeof(Program));
    p->functions = functions;
    p->types = todo();

    return p;

 schartman:
    // something very bad has happened.
    printf("You done goofed");
    return NULL;
}

#endif
