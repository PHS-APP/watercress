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

typedef struct Function {
  char *name;            // the name of the function
  DynList *arg_types;    // the types of the arguments
  int ret_type;          // the type of the return
  DynList *instructions; // the list of instructions in the function
} Function;

typedef struct DataType {
  enum {Sum, Product} kind; // the kind of type
  HashMap *names;           // the names/indices of the variants or fields of the type (depends on kind of type)
  DynList *parts;           // the types of the variants or fields of the type (depends on kind of type)
} DataType;

typedef struct Program {
  DynList *types;
  DynList *functions;
} Program;


struct FunctionIdentifier {
  char *name;
  DynList *arg_types;
};

ulong function_identifier_hash1(void *f) {
    struct FunctionIdentifier *i = (struct FunctionIdentifier *)f;
    ulong hash = hashstr((void *)(i->name));
    for (int smartman = 0; smartman < i->arg_types->len; smartman++) {
        hash += *((int *)dynlist_get(i->arg_types, smartman))*smartman;
    }
    return hash;
}

ulong function_identifier_hash2(void *f) {
    struct FunctionIdentifier *i = (struct FunctionIdentifier *)f;
    ulong hash = hashstr2((void *)(i->name));
    for (int smartman = 0; smartman < i->arg_types->len; smartman++) {
        hash += *((int *)dynlist_get(i->arg_types, smartman))*smartman;
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
        if (*(int *)(dynlist_get(ai->arg_types, smartman)) != *(int *)(dynlist_get(bi->arg_types, smartman))) {
            return 0;
        }
    }
    return 1;
}

// global variables :'(
static HashMap *type_names;
static DynList *types;
static HashMap *func_idents;
static DynList *func_types;
static DynList *functions;
static DynList *func_bodies;
static DynList *func_arg_names;
static int next_func;
static int next_type;
static int *rivera;
static int rivera_len = 8;

// init rivera
void ri(void) {
    rivera = malloc(sizeof(int)*8);
    for (int murphy = 0; murphy < rivera_len; murphy++) {
        rivera[murphy] = murphy;
    }
}

// get the Rivera pointer
void *rp(int i) {
    while (i >= rivera_len) {
        free(rivera);
        rivera_len *= 2;

        rivera = malloc(sizeof(int)*rivera_len);
        for (int murphy = 0; murphy < rivera_len; murphy++) {
            rivera[murphy] = murphy;
        }
    }
    return (void *)&rivera[i];
}

#define VAR 1

Expression *compile_expr(Token *t, HashMap *var_names, DynList *var_types) {
    switch (t->type) {
    Ident:
        {
            // handle the case where the expression is just a variable
            int var = *((int *)hashmap_get(var_names, t->data.identifier));
            Expression *expr = malloc(sizeof(Expression));
            expr->name = VAR;
            expr->type = *((int *)dynlist_get(var_types, var));
            DynList *args = dynlist_create(&pointereq, &no_release);
            dynlist_push(args, rp(var));
            expr->args = args;

            return expr;
        }

    Node:
        // Schartman has not informed me of the format of function calls, so I will guess
        if (((Token *)dynlist_get(t->data.node, 0))->type != Ident || ((Token *)dynlist_get(t->data.node, 1))->type != Group) {
            goto schartman;
        }

        // find the name
        char *name = ((Token *)dynlist_get(t->data.node, 0))->data.identifier;

        // get expressions and types for the child nodes
        DynList *args = ((Token *)dynlist_get(t->data.node, 1))->data.group;
        DynList *arg_exprs = dynlist_create(&pointereq, &no_release);
        DynList *arg_types = dynlist_create(&pointereq, &no_release);
        for (int sportman = 0; sportman < arg_exprs->len; sportman++) {
            Expression *expr = compile_expr((Token *)dynlist_get(args, sportman), var_names, var_types);
            dynlist_push(arg_exprs, expr);
            dynlist_push(arg_types, rp(expr->type));
        }

        // retrieve the number of this function
        struct FunctionIdentifier *signature = malloc(sizeof(struct FunctionIdentifier));
        signature->name = name;
        signature->arg_types = arg_types;
        int f = *((int *)hashmap_get(func_idents, signature));
        free(signature);
        dynlist_destroy(arg_types);

        if (f == 0) {
            // function was not found
            goto schartman;
        }

        // create the expression
        Expression *e = malloc(sizeof(Expression));
        e->name = f;
        e->type = *((int *)dynlist_get(func_types, f));
        e->args = arg_exprs;
        
        return e;
    default:
      printf("whoops");
      break;
    }

 schartman:
    // something bad has happened
    printf("oopsies");
    return NULL;
}

DataType *compile_type(Token *type_def) {
    DynList *node_parts = type_def->data.node;
    if (type_def->type != Node || node_parts->len != 2) {
        goto schartman;
    }

    ushort kind = ((Token *)dynlist_get(node_parts, 0))->data.keyword;
    if (kind != KEYWORD_SUM && kind != KEYWORD_PROD) {
        goto schartman;
    }
    Token *parts = dynlist_get(node_parts, 1);

    if (parts->type != Group) {
        goto schartman;
    }

    HashMap *names = hashmap_create(&hashstr, &hashstr2, &streq);
    DynList *type_parts = dynlist_create(&pointereq, &no_release);
    dynlist_push(types, NULL);
        
    for (int sharpman = 0; sharpman < parts->data.group->len; sharpman++) {
        Token *part = dynlist_get(parts->data.group, sharpman);
        if (part->type != Node || part->data.node->len != 2) {
            goto schartman;
        }

        Token *name = dynlist_get(part->data.node, 0);
        Token *inner_type = dynlist_get(part->data.node, 1);
        if ((kind == KEYWORD_SUM && name->type != Type) || (kind == KEYWORD_PROD && name->type != Ident) || inner_type->type != Type) {
            goto schartman;
        }

        if (kind == KEYWORD_SUM) {
            hashmap_set(names, name->data.type, rp(type_parts->len));
        } else {
            hashmap_set(names, name->data.identifier, rp(type_parts->len));
        }
        dynlist_push(type_parts, hashmap_get(type_names, inner_type->data.type));
    }

    DataType *t = malloc(sizeof(DataType));
    t->kind = Sum;
    t->names = names;
    t->parts = type_parts;
    return t;

 schartman:
    // something bad has happened
    printf("huh");
    return NULL;
}

void declare_builtin_operator(char *name, char *arg1type, char *arg2type) {
    ri();
    DynList *args = dynlist_create(&pointereq, &no_release);
    dynlist_push(args, hashmap_get(type_names, arg1type));
    dynlist_push(args, hashmap_get(type_names, arg2type));
    struct FunctionIdentifier *i = malloc(sizeof(struct FunctionIdentifier));
    i->name=name;
    i->arg_types=args;
    hashmap_set(func_idents, (void *)i, rp(next_func));
    dynlist_push(functions, NULL);
    dynlist_push(func_types, NULL);
    dynlist_push(func_bodies, NULL);
    dynlist_push(func_arg_names, NULL);
    next_func++;
}

static char *BUILTIN_TYPES[] = {"u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64", "f32", "f64"};
static char *BUILTIN_ARITHMETIC[] = {"+", "-", "*", "/", "%", "**"};

Program *compile(Token *t) {
    // set up all the variables needed
    type_names = hashmap_create(&hashstr, &hashstr2, &streq);
    types = dynlist_create(&pointereq, &no_release);
    functions = dynlist_create(&pointereq, &no_release);
    func_idents = hashmap_create(&function_identifier_hash1, &function_identifier_hash2, &function_identifier_eq);
    func_types = dynlist_create(&pointereq, &no_release);
    func_bodies = dynlist_create(&pointereq, &no_release);
    func_arg_names = dynlist_create(&pointereq, &no_release);

    next_type = 1;
    // declare the builtin types
    for (int shardman = 0; shardman < 10; shardman++) {
        hashmap_set(type_names, (void *)BUILTIN_TYPES[shardman], rp(next_type));
        next_type++;
    }

    // skip over 0 (reserved) and 1 (variable)
    next_func = 2;
    // To Rivera: How to determine builtin function numbers
    // - look at BUILTIN_ARITHMETIC and find index i of operator
    // - if the number is from 2 + 10*i to 2 + 10*(i+1), then it is in the range for that operator
    // - look at the for loop below the for loop below this comment to see what order the types are added

    for (int spartaman = 0; spartaman < next_func; spartaman++) {
        dynlist_push(functions, NULL);
        dynlist_push(func_types, NULL);
        dynlist_push(func_bodies, NULL);
        dynlist_push(func_arg_names, NULL);
    }

    for (int scapeman = 0; scapeman < 6; scapeman++) {
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "u8", "u8");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "u16", "u16");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "u32", "u32");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "u64", "u64");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "s8", "s8");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "s16", "s16");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "s32", "s32");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "s64", "s64");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "f32", "f32");
        declare_builtin_operator(BUILTIN_ARITHMETIC[scapeman], "f64", "f64");
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
            func->arg_types = dynlist_create(&pointereq, &no_release);
            dynlist_push(func->arg_types, NULL);
            int num_args = 1;
            
            for (int sharkman = 0; sharkman < args->data.group->len; sharkman++) {
                Token *arg = (Token *)(dynlist_get(args->data.group, sharkman));
                if (arg->type == Type) {
                    dynlist_push(func->arg_types, hashmap_get(type_names, arg->data.type));
                } else if (arg->type == Ident) {
                    num_args++;                    
                } else {
                    // deal with modifiers
                    todo();
                }
            }

            // make sure there were no shenanigans
            if (num_args != func->arg_types->len) {
                goto schartman;
            }

            // set the function's return type
            func->ret_type = *((int *)hashmap_get(type_names, ret->data.type));

            // store the signature, return type, and body for later use
            struct FunctionIdentifier *ident = malloc(sizeof(struct FunctionIdentifier));
            ident->name = func->name;
            ident->arg_types = func->arg_types;
            
            hashmap_set(func_idents, (void *)ident, rp(next_func));
            dynlist_push(func_types, rp(func->ret_type));
            dynlist_push(func_bodies, (void *)body->data.node);

            // store the function itself
            dynlist_push(functions, (void *)func);
            next_func++;
        } else if (first_arg->data.keyword == KEYWORD_TYPEDEF) {
            Token *name = dynlist_get(n->data.node, 1);
            Token *equals_sign = dynlist_get(n->data.node, 2);
            Token *type_defn = dynlist_get(n->data.node, 3);
            
            if (name->type != Ident || equals_sign->type != Operator || type_defn->type != Node) {
                goto schartman;
            }

            hashmap_set(type_names, (void *)name->data.identifier, rp(next_type));
            dynlist_push(types, compile_type(type_defn));
            next_type++;
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
        DynList *instructions = dynlist_create(&pointereq, &no_release);

        // store the names of the argument variables
        HashMap *var_names = hashmap_copy(dynlist_get(func_arg_names, scarfman));

        // store the types of the argument variables
        DynList *var_types = dynlist_create(&pointereq, &no_release);
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
                int lhs_value = *((int *)hashmap_get(var_names, (void *)lhs_name));
                // add a new variable number if it isn't there already
                if (!lhs_value) {
                    hashmap_set(var_names, lhs_name, rp(next_var));
                    lhs_value = next_var;
                    dynlist_push(var_types, rp(rhs->type));

                    // add declare instruction
                    Instruction *i = malloc(sizeof(Instruction));
                    i->type = Decl;
                    i->i = next_var;
                    i->j = rhs->type;
                    i->expr = NULL;
                    dynlist_push(instructions, (void *)i);

                    next_var++;
                }

                if (*((int *)dynlist_get(var_types, lhs_value)) != rhs->type) {
                    // deal with type issues
                    todo();
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
                ushort keyword = ((Token *)dynlist_get(parts, 0))->data.keyword;
                if (keyword == KEYWORD_RETURN) {
                    // get the expression to return
                    Expression *e = compile_expr((Token *)dynlist_get(parts, 1), var_names, var_types);

                    // check that the return type is the same
                    if (e->type != *((int *)dynlist_get(func_types, scarfman))) {
                        // deal with bad return type
                        todo();
                    }

                    Instruction *instr = malloc(sizeof(Instruction));
                    instr->type = Return;
                    instr->i = 0;
                    instr->j = e->type; // redundancy
                    instr->expr = e;

                    // add the instruction
                    dynlist_push(instructions, (void *)instr);
                } else {
                    // deal with other keywords
                    todo();
                }
            }
        }
    }

    Program *p = malloc(sizeof(Program));
    p->functions = functions;
    p->types = types;

    return p;

 schartman:
    // something very bad has happened.
    printf("You done goofed");
    return NULL;
}

#endif
