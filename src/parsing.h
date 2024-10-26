#ifndef __PARSING_H__
#define __PARSING_H__ 1
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "./tokens.h"
#include "./utils.h"

#define LINKED_OBJECTS linkedlist_create(pointereq)
void print_semantic_token(Token*, char*);

char DEBUG_ESCAPES = 0;

static char inited = 0;
// list of processed files
static LinkedList* processed = NULL;

static int inoeq(void* p1, void* p2) {
    return *((ino_t*)p1) == *((ino_t*)p2);
}

void parser_init(void) {
    if (inited) return;
    inited = 1;
    processed = linkedlist_create((ItemEq)inoeq);
}

static int get_ino(char* fpath, ino_t* ino) {
    int r = 0;
    struct stat* statbuf = (struct stat*)malloc(sizeof(struct stat));
    if (stat(fpath, statbuf) == -1) {
        r = -1;
        goto release;
    }
    *ino = statbuf->st_ino;
    release:
    free(statbuf);
    return r;
}

/*
returns 1 if the file has already been processed
*/
static int check_file(char* fpath) {
    int ret = 0;
    ino_t ino;
    if (get_ino(fpath, &ino) == -1) {
        goto bottom;
    }
    if (linkedlist_indexof(processed, &ino) != 0) {
        ret = 1;
    }
    bottom:
    return ret;
}

static char* CONTENTTYPEMAP[] = {"NONE", "STR", "WORD", "INT", "FLOT", "SYM", "CHAR", "LINE", "PROC"};

#define CONTENT_NONE 0
#define CONTENT_STR  1
#define CONTENT_WORD 2
#define CONTENT_INT  3
#define CONTENT_FLOT 4
#define CONTENT_SYM  5
#define CONTENT_CHAR 6
#define CONTENT_LINE 7
// already processed tokens
#define CONTENT_PROC 8

#define CONTENT_NOTSTR(c) ((c!=CONTENT_STR)&&(c!=CONTENT_CHAR))

#define BASE_10 0
#define BASE_2  1
#define BASE_8  2
#define BASE_16 3

#define IS_ALPHA(c) ((c>64&&c<91)||(c>96&&c<123))
#define IS_NUMER(c) (c>47&&c<58)
#define IS_ALNUM(c) (IS_ALPHA(c)||IS_NUMER(c))
#define IS_WORDC(c) (IS_ALNUM(c)||(c=='_'))
#define IS_SYMBL(c) ((c>=32)&&!IS_WORDC(c))

static int reval_content(char c) {
    if (IS_ALPHA(c) || c == '_') return CONTENT_WORD;
    if (IS_NUMER(c)) return CONTENT_INT;
    if (IS_SYMBL(c)) {
        if (c == ' ') return CONTENT_NONE;
        if (c == '"') return CONTENT_STR;
        if (c == '\'') return CONTENT_CHAR;
        return CONTENT_SYM;
    }
    return CONTENT_NONE;
}

static long parse_hex(char* hexstr) {
    long acc = 0;
    int cnt = 0;
    for (int i = strlen(hexstr)-1; i >= 0; i --) {
        char c = hexstr[i];
        long digit = 0;
        if (IS_NUMER(c)) {
            digit = c - '0';
        } else {
            digit = ((c|32) - 'a') + 10;
        }
        acc |= (digit << ((cnt++)*4));
    }
    return acc;
}
static long parse_oct(char* octstr) {
    long acc = 0;
    int cnt = 0;
    for (int i = strlen(octstr)-1; i >= 0; i --) {
        char c = octstr[i];
        long digit = c - '0';
        acc |= (digit << ((cnt++)*3));
    }
    return acc;
}
static long parse_bin(char* binstr) {
    long acc = 0;
    int cnt = 0;
    for (int i = strlen(binstr)-1; i >= 0; i --) {
        char c = binstr[i];
        acc |= ((c == '0' ? 0 : 1) << (cnt++));
    }
    return acc;
}
static double parse_floatstr(char* str) {
    double acc = 0;
    size_t dot = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i ++) {
        if (str[i] == '.') {
            dot = i;
            break;
        }
    }
    for (size_t i = 0; i < len; i ++) {
        if (i == dot) continue;
        double digit = (double)(str[i]-'0');
        if (i < dot) {
            size_t cnt = dot-i-1;
            for (size_t j = 0; j < cnt; j ++) {
                digit *= 10;
            }
        } else {
            size_t cnt = i-dot;
            for (size_t j = 0; j < cnt; j ++) {
                digit /= 10;
            }
        }
        acc += digit;
    }
    return acc;
}
static long parse_intstr(char* str) {
    size_t len = strlen(str);
    if (len > 1 && !IS_NUMER(str[1])) {
        int base = (str[1] == 'x') ? BASE_16 : ((str[1] == 'b') ? BASE_2 : BASE_8);
        if (base == BASE_16) {
            return parse_hex(str+2);
        }
        if (base == BASE_8) {
            return parse_oct(str+2);
        }
        return parse_bin(str+2);
    }
    long acc = 0;
    int pos = len-1;
    for (int i = 0; i < len; i ++) {
        long digit = (long)(str[i]-'0');
        for (int j = 0; j < pos; j ++) {
            digit *= 10;
        }
        pos --;
        acc += digit;
    }
    return acc;
}
static int resolve_charstr(char* str) {
    if (str[0] != '\\') {
        return (int)str[0];
    }
    switch(str[1]) {
        case'\n':case'\r':case'\t':case'\\':case'\'':
        return(int)str[1];
        default:break;
    }
    if (str[1] == 'x') {
        int r = (int)parse_hex(str+2);
        if (DEBUG_ESCAPES) printf("ESCRES_DBG:\nstr=%s (hex)\nr=%x\n", str, r);
        return r;
    }
    if (str[1] != 'u' && str[1] != 'w') return 0x1a;
    if (str[1] == 'u' && strlen(str) != 6) return 0x1a;
    if (str[1] == 'w' && strlen(str) != 10) return 0x1a;
    long e = parse_hex(str+2);
    long c1 = e >> 16, c2 = e & 0xffff;
    int r = 0x1a;
    if (c1 == 0) {
        if ((c2 & ~0x7f) == 0) {
            r = (int)(c2&0xff);
            if (DEBUG_ESCAPES) printf("ESCRES_DBG:\nstr=%s (utf8:1)\npv=%lx\nc1=%lx  c2=%lx\nr=%x\n", str, e, c1, c2, r);
        } else {
            if ((c2 & ~0x7ff) == 0) {
                r = (int)(((0xc0|(c2>>6))<<8)|(0x80|(c2&0x3f)));
                if (DEBUG_ESCAPES) printf("ESCRES_DBG:\nstr=%s (utf8:2)\npv=%lx\nc1=%lx  c2=%lx\nr=%x\n", str, e, c1, c2, r);
            } else {
                r = (int)(((0xe0|(c2>>12))<<16)|((0x80|(c2>>6))<<8)|(0x80|(c2&0x3f)));
                if (DEBUG_ESCAPES) printf("ESCRES_DBG:\nstr=%s (utf8:3)\npv=%lx\nc1=%lx  c2=%lx\nr=%x\n", str, e, c1, c2, r);
            }
        }
    } else {
        r = (int)(((0xf0|(c1>>2))<<24)|((0x80|((c1&0x3)<<4)|(c2>>12))<<16)|((0x80|((c2&0xfff)>>6))<<8)|(0x80|(c2&0x3f)));
        if (DEBUG_ESCAPES) printf("ESCRES_DBG:\nstr=%s (utf8:4)\npv=%lx\nc1=%lx  c2=%lx\nr=%x\n", str, e, c1, c2, r);
    }
    return r;
}
static char* resolve_strstr(char* str) {
    StrBuf* buf = strbuf_create();
    StrBuf* b2 = strbuf_create();
    int esc = 0;
    int cnt = 0;
    char c = 0;
    while ((c = *(str++))) {
        if (esc) {
            if (esc == 1) {
                esc = 0;
                strbuf_push(b2, c);
                if (c == 'x') {
                    esc = 2;
                    cnt = 2;
                } else if (c == 'u') {
                    esc = 3;
                    cnt = 4;
                } else if (c == 'w') {
                    esc = 4;
                    cnt = 8;
                } else {
                    strbuf_discard(b2);
                    if (c == 'n') {
                        strbuf_push(buf, '\n');
                    } else if (c == 'r') {
                        strbuf_push(buf, '\r');
                    } else if (c == 't') {
                        strbuf_push(buf, '\t');
                    } else if (c == '"') {
                        strbuf_push(buf, '"');
                    } else if (c == '\\') {
                        strbuf_push(buf, '\\');
                    }
                }
            } else {
                if (cnt--) {
                    strbuf_push(b2, c);
                }
                if (cnt == 0) {
                    int cbytes = resolve_charstr(b2->ptr);
                    strbuf_discard(b2);
                    for (int i = 0; i < 4; i ++) {
                        int v = (cbytes>>((3-i)*8))&0xff;
                        if (v != 0 || i == 3) {
                            strbuf_push(buf, (char)v);
                        }
                    }
                    esc = 0;
                }
            }
        } else if (c == '\\') {
            esc = 1;
            strbuf_push(b2, '\\');
        } else {
            strbuf_push(buf, c);
        }
    }
    char* ret = strbuf_consume(buf);
    strbuf_destroy(buf);
    strbuf_destroy(b2);
    return ret;
}

static void parsing_error(const char* msg, long line, long col, const char* file) {
    printf("PARSE ERROR: %s\nSOURCE: %li, %li of %s\n", msg, line, col, file);
    exit(1);
}

typedef union ParserData {
    char line;
    int character;
    char *string, *word, *symbol;
    long integer;
    double floating;
    DynList* processed;
} ParserData;

typedef struct ParserToken {
    ubyte chk;
    int type;
    long line, column;
    char* file;
    ParserData data;
} ParserToken;

static ParserToken BADTOK = {.chk=2,.column=-1,.line=-1,.file="NOFILE",.type=7,.data={.line='\n'}};

static void printutf8(int bytes) { // prints utf8 of up to four bytes
    if (DEBUG_ESCAPES) {
        printf("%x", bytes);
        return;
    }
    for (int i = 0; i < 4; i ++) {
        int v = (bytes>>((3-i)*8))&0xff;
        if (v != 0 || i == 3) {
            printf("%c", (char)v);
        }
    }
}

static ParserToken* create_parsertoken(int type, long line, long column, char* file, ParserData data) {
    ParserToken* tok = (ParserToken*)malloc(sizeof(ParserToken));
    tok->chk = 2;
    tok->type = type;
    tok->line = line;
    tok->column = column;
    tok->file = file;
    tok->data = data;
    return tok;
}
void parsertoken_print(ParserToken* tok) {
    printf("PToken {\n    type: %s,\n    loc: (%li, %li, %s),\n", CONTENTTYPEMAP[tok->type], tok->line, tok->column, tok->file);
    if (tok->type != CONTENT_PROC) {
        printf("    ");
    }
    switch (tok->type) {
        case CONTENT_WORD:case CONTENT_SYM:
        case CONTENT_STR:printf("value: %s\n", tok->data.string);break;
        case CONTENT_CHAR:printf("value: ");printutf8(tok->data.character);printf("\n");break;
        case CONTENT_INT:printf("value: %li\n", tok->data.integer);break;
        case CONTENT_FLOT:printf("value: %f\n", tok->data.floating);break;
        case CONTENT_LINE:printf("no value\n");break;
        case CONTENT_PROC:for(int i=0;i<tok->data.processed->len;i++){print_semantic_token(tok->data.processed->ptr[i], "    ");}break;
        default:printf("unrecognized token type id\n");break;
    }
    printf("}\n");
}
void parsertoken_print_condensed(ParserToken* tok) {
    printf("(%s, %li:%li, ", CONTENTTYPEMAP[tok->type], tok->line, tok->column);
    switch (tok->type) {
        case CONTENT_WORD:case CONTENT_SYM:
        case CONTENT_STR:printf("'%s'", tok->data.string);break;
        case CONTENT_CHAR:printf("'");printutf8(tok->data.character);printf("'");break;
        case CONTENT_INT:printf("'%li'", tok->data.integer);break;
        case CONTENT_FLOT:printf("'%f'", tok->data.floating);break;
        case CONTENT_LINE:printf("no value");break;
        default:printf("???");break;
    }
    printf(") ");
}

static int check_symbol_continuity(StrBuf* buf, char new) {
    if (!IS_SYMBL(new)) return 0;
    if (buf->len <= 2 && buf->len > 0) {
        if (new == '=') {
            if (buf->len == 1) {
                switch (buf->ptr[0]) { // v [x]= m
                    case'=':case'^':case'%':case'!':case'&':case'*':case'-':
                    case'+':case'/':case'<':case'>':case'~':case'|':
                    return 1;
                    default:return 0;
                }
            }
            switch (buf->ptr[0]) { // >>= <<= **=
                case'>':case'*':case'<':
                return buf->ptr[0]==buf->ptr[1];
                default:return 0;
            }
        }
        if (buf->len == 1 && new == buf->ptr[0]) {
            switch (new) { // ++ -- ** >> << || &&
                case'+':case'-':case'*':case'>':case'<':case'|':case'&':
                return 1;
                default:return 0;
            }
        }
    }
    return 0;
}

#define PHASE_INCL 0
#define PHASE_GLBL 1
#define PHASE_TYPE 2
#define PHASE_FUNC 3

// static void parsertoken_print(ParserToken*);

static void _transform_error(char* msg, ParserToken* cause, long line, const char* func, const char* file) {
    printf("%li, %s{%s}\n", line, func, file);
    printf("Semantic Transformation Error: %s (%li, %li, %s)\n", msg, cause->line, cause->column, cause->file);
    parsertoken_print(cause);
    exit(1);
}
#define transform_error(msg, tok) _transform_error(msg, tok, __LINE__, __func__, __FILE__)

static int rephase(int op, ParserToken* tok) { // determines the next phase
    if (op < PHASE_GLBL) {
        if (!strcmp(tok->data.word, "global") || !strcmp(tok->data.word, "const")) {
            return PHASE_GLBL;
        }
    }
    if (op < PHASE_TYPE) {
        if (!(strcmp(tok->data.word, "typedef") && strcmp(tok->data.word, "pattern"))) {
            return PHASE_TYPE;
        }
    }
    if (op < PHASE_FUNC) {
        if (!strcmp(tok->data.word, "func") || !strcmp(tok->data.word, "impure") || !strcmp(tok->data.word, "foreign")) {
            return PHASE_FUNC;
        }
    }
    transform_error("invalid phase transition", tok);
    return -1;
}

static Token* _smart_create_token(TokenType type, long line, long column, char* file, long data) {
    TokenData d = {0};
    switch(type) {
        case Node:d.node=(DynList*)data;break;
        case Nmsp:{NmspData nd={.name=(char*)data,.childnode=dynlist_create(pointereq,no_release)};d.namespace=nd;}break;
        case Generic:{GeniData gd={.name=(char*)data,.restrictions=dynlist_create(pointereq,no_release)};d.generic=gd;};break;
        case Ident:d.identifier=(char*)data;break;
        case Bool:d.boolean=(char)data;break;
        case Int:d.integer=data;break;
        case Float:d.floating=*((double*)&data);break;
        case Keyword:d.keyword=(ushort)data;break;
        case Operator:d.operator=(char)data;break;
        case Char:d.character=(char)data;break;
        case String:d.string=(char*)data;break;
        case Group:d.group=(DynList*)data;break;
        case Type:d.type=(char*)data;break;
        case Mod:d.modifier=(ushort)data;break;
        default:break;
    }
    return token_create(type, line, column, file, d);
}
#define smart_create_token(type, line, column, file, data) _smart_create_token(type, line, column, file, (long)data)
#define smart_create_tokenl(type, loc, data) _smart_create_token(type, loc, (long)data)
#define smart_create_tokent(type, tok, data) _smart_create_token(type, ((ParserToken*)tok)->line, ((ParserToken*)tok)->column, ((ParserToken*)tok)->file, (long)data)

#define SEC_INCL_NONE 0
#define SEC_INCL_USED 1
#define SEC_INCL_NAME 2

#define SEC_GLBL_NONE 0

#define SEC_TYPE_NONE 0
#define SEC_TYPE_NAME 1
#define SEC_TYPE_SPEC 2
#define SEC_TYPE_INLN 3
#define SEC_TYPE_BODY 4
#define SEC_PATT_NAME 5
#define SEC_PATT_BODY 6

#define SEC_FUNC_NONE 0

#define tokloc tok->line, tok->column, tok->file
#define noloc -1, -1, treename
static Token* transform_proc_generic(char* treename, DynList* tokens) {
    DynList* final = dynlist_create(pointereq, no_release);
    int part = 0;
    int depth = 0;
    int s, e;
    ParserToken* held;
    // printf("TOKENS DETAILS: %d, %d, %p\n", tokens->len, tokens->cap, (void*)tokens->ptr);
    // for (int i = 0; i < tokens->cap; i ++) {
    //     printf("(%p) ", tokens->ptr[i]);
    // }
    // printf("\n");
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        // printf("CHECK: %d\n", tok->chk);
        // parsertoken_print_condensed(tok);
        if (part == 0) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "[")) {
                    depth = 1;
                    part = 6;
                    held = tok;
                    continue;
                }
            }
            if (tok->type != CONTENT_WORD) {
                transform_error("expected type name", tok);
            }
            dynlist_push(final, smart_create_tokenl(Type, tokloc, strmove(tok->data.word)));
            part = 1;
        } else if (part == 1) {
            if (tok->type != CONTENT_WORD) {
                transform_error("expected word", tok);
            }
            if (strcmp(tok->data.word, "of")) {
                transform_error("expected generic specifier", tok);
            }
            part = 2;
        } else if (part == 2) {
            if (tok->type != CONTENT_WORD) {
                transform_error("expected generic parameter", tok);
            }
            dynlist_push(final, smart_create_tokenl(Generic, tokloc, strmove(tok->data.word)));
            part = 3;
        } else if (part == 3) {
            if (tok->type == CONTENT_WORD) {
                if (strcmp(tok->data.word, "and")) {
                    transform_error("unexpected word", tok);
                }
                part = 2;
            } else {
                if (tok->type != CONTENT_SYM) {
                    transform_error("expected symbol", tok);
                }
                if (strcmp(tok->data.symbol, "=")) {
                    transform_error("expected '='", tok);
                }
                part = 4;
            }
        } else if (part == 4) {
            if (tok->type == CONTENT_SYM) {
                if (strcmp(tok->data.symbol, "(")) {
                    transform_error("invalid symbol", tok);
                }
                depth = 1;
                part = 5;
                s = i+1;
            } else {
                if (tok->type != CONTENT_WORD) {
                    transform_error("invalid token", tok);
                }
                if (!(strcmp(tok->data.word, "of") && strcmp(tok->data.word, "and") && strcmp(tok->data.word, "is"))) {
                    transform_error("invalid keyword", tok);
                }
                dynlist_push(((Token*)final->ptr[final->len-1])->data.generic.restrictions, smart_create_tokenl(Type, tokloc, strmove(tok->data.word)));
                part = 3;
            }
        } else if (part == 5) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth ++;
                }
                if (!strcmp(tok->data.symbol, ")")) {
                    depth --;
                }
                if (depth == 0) {
                    e = i;
                    if (s == e) {
                        transform_error("empty specifier", tok);
                    }
                    DynList window = {.eq=pointereq,.rel=no_release,.cap=0,.len=(e-s),.ptr=(tokens->ptr+s)};
                    dynlist_push(((Token*)final->ptr[final->len-1])->data.generic.restrictions, transform_proc_generic(treename, &window));
                    part = 3;
                }
            }
        } else if (part == 6) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "[")) {
                    depth ++;
                }
                if (!strcmp(tok->data.symbol, "]")) {
                    depth --;
                }
                if (depth == 0) {
                    e = i;
                    if (s == e) {
                        transform_error("empty list expression", tok);
                    }
                    DynList window = {.eq=pointereq,.rel=no_release,.cap=0,.len=(e-s),.ptr=(tokens->ptr+s)};
                    dynlist_push(final, transform_proc_generic(treename, &window));
                    dynlist_push(final, smart_create_tokent(Mod, held, MODIF_ARRAY));
                    break;
                }
            }
        }
    }
    // printf("\n");
    Token* g = smart_create_token(Group, -1, -1, treename, final);
    // print_semantic_token(g, "");
    return g;
}

static Token* transform_proc_defbod(char* treename, DynList* tokens, char kind) {
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* buffer = dynlist_create(pointereq, no_release);
    Token* g = smart_create_token(Group, -1, -1, treename, dynlist_create(pointereq, no_release));
    int part = 0;
    int s, e, depth;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        if (part == 0) {
            if (tok->type == CONTENT_LINE) continue;
            if (tok->type != CONTENT_WORD) {
                transform_error("expected word", tok);
            }
            dynlist_push(g->data.group, smart_create_tokenl(Ident, tokloc, strmove(tok->data.word)));
            if (kind == 1) {
                part = 3;
            } else {
                part = 1;
            }
        } else if (part == 1) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, ":")) {
                    part = 2;
                    continue;
                }
            }
            transform_error("expected ':'", tok);
        } else if (part == 2) {
            if (tok->type == CONTENT_LINE) {
                dynlist_push(g->data.group, transform_proc_generic(treename, buffer));
                dynlist_push(final, g);
                g = smart_create_tokenl(Group, noloc, dynlist_create(pointereq, no_release));
                dynlist_clear(buffer);
                part = 0;
                continue;
            }
            dynlist_push(buffer, tok);
        } else if (part == 3) {
            if (tok->type == CONTENT_LINE) {
                part = 0;
                dynlist_push(final, g);
                g = smart_create_tokenl(Group, noloc, dynlist_create(pointereq, no_release));
                dynlist_clear(buffer);
                continue;
            }
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth = 1;
                    part = 4;
                    continue;
                }
            }
        } else if (part == 4) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth ++;
                } else if (!strcmp(tok->data.symbol, ")")) {
                    depth --;
                }
                if (depth == 0 || (depth == 1 && !strcmp(tok->data.symbol, ","))) {
                    dynlist_push(g->data.group, transform_proc_generic(treename, buffer));
                    dynlist_clear(buffer);
                    if (depth == 0) {
                        dynlist_push(final, g);
                        g = smart_create_tokenl(Group, noloc, dynlist_create(pointereq, no_release));
                        part = 0;
                    }
                    continue;
                }
            }
            dynlist_push(buffer, tok);
        }
    }
    dynlist_destroy(buffer);
    dynlist_destroy(g->data.group);
    free(g);
    return smart_create_tokenl(Group, noloc, final);
}
static Token* transform_proc_fparam(char* treename, DynList* tokens) {
    DynList* lst = dynlist_create(pointereq, no_release);
    int part = 0;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        if (part == 0) {
            if (tok->type != CONTENT_WORD) {
                transform_error("expected parameter name", tok);
            }
            dynlist_push(lst, smart_create_tokenl(Ident, tokloc, strmove(tok->data.word)));
            part = 1;
        } else if (part == 1) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, ":")) {
                    DynList window = {.eq=pointereq,.rel=no_release,.len=(tokens->len-2),.cap=0,.ptr=(tokens->ptr+2)};
                    if (window.len == 0) {
                        transform_error("incomplete parameter type specification", tok);
                    }
                    dynlist_push(lst, transform_proc_generic(treename, &window));
                    break;
                }
            }
            transform_error("expected ':'", tok);
        }
    }
    return smart_create_tokenl(Group, noloc, lst);
}
static Token* transform_proc_fncdef(char* treename, DynList* tokens) {
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* buffer = dynlist_create(pointereq, no_release);
    int part = 0;
    int depth = 0;
    ParserToken* held = NULL;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        if (part == 0) {
            if (tok->type != CONTENT_WORD) {
                transform_error("expected word", tok);
            }
            if (!strcmp(tok->data.word, "impure")) {
                if (final->len > 0) {
                    transform_error("multiple modifirs disallowed", tok);
                }
                held = tok;
            } else if (!strcmp(tok->data.word, "func")) {
                dynlist_push(final, smart_create_tokenl(Keyword, tokloc, KEYWORD_FUNC));
                if (held != NULL) {
                    dynlist_push(final, smart_create_tokent(Mod, held, MODIF_IMPURE));
                }
                part = 1;
            } else {
                transform_error("expected keyword", tok);
            }
        } else if (part == 1) {
            if (tok->type != CONTENT_WORD) {
                transform_error("expected function name", tok);
            }
            dynlist_push(final, smart_create_tokenl(Ident, tokloc, strmove(tok->data.word)));
            part = 2;
        } else if (part == 2) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    part = 3;
                    depth = 1;
                    continue;
                }
            }
            transform_error("expected '('", tok);
        } else if (part == 3) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth ++;
                } else if (!strcmp(tok->data.symbol, ")")) {
                    depth --;
                }
                if (depth == 0 || (depth == 1 && !strcmp(tok->data.symbol, ","))) {
                    dynlist_push(final, transform_proc_fparam(treename, buffer));
                    dynlist_clear(buffer);
                    if (depth == 0) {
                        part = 4;
                    }
                    continue;
                }
            }
            dynlist_push(buffer, tok);
        } else if (part == 4) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, ":")) {
                    part = 5;
                    continue;
                }
                transform_error("expected ':'", tok);
            }
            if (tok->type == CONTENT_WORD) {
                if (!strcmp(tok->data.word, "of")) {
                    part = 6;
                    ParserData d = {.word="##"};
                    dynlist_push(buffer, create_parsertoken(CONTENT_WORD, -1, -1, treename, d));
                    dynlist_push(buffer, tok);
                    continue;
                }
                transform_error("expected 'of'", tok);
            }
            transform_error("expected ':' or 'of'", tok);
        } else if (part == 5) {
            if (tok->type == CONTENT_LINE) {
                // for (int j = 0; j < buffer->len; j ++) {
                //     parsertoken_print_condensed(dynlist_get(buffer, j));
                // }
                // printf("\n");
                dynlist_push(final, transform_proc_generic(treename, buffer));
                dynlist_clear(buffer);
                break;
            } else {
                dynlist_push(buffer, tok);
            }
        } else if (part == 6) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, ":")) {
                    Token* t = transform_proc_generic(treename, buffer);
                    free(((Token*)t->data.group->ptr[0])->data.identifier);
                    free(dynlist_remove(t->data.group, 0));
                    dynlist_push(final, t);
                    dynlist_clear(buffer);
                    part = 5;
                    continue;
                }
            }
            dynlist_push(buffer, tok);
        }
    }
    // for (int j = 0; j < buffer->len; j ++) {
    //     parsertoken_print_condensed(dynlist_get(buffer, j));
    // }
    // printf("\n");
    if (buffer->len > 0) {
        dynlist_push(final, transform_proc_generic(treename, buffer));
    }
    dynlist_destroy(buffer);
    return smart_create_tokenl(Node, noloc, final);
}
static Token* transform_proc_patbod(char* treename, DynList* tokens) {
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* buffer = dynlist_create(pointereq, no_release);
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        if (tok->type == CONTENT_LINE) {
            if (buffer->len > 0) {
                dynlist_push(final, transform_proc_fncdef(treename, buffer));
                dynlist_clear(buffer);
            }
        } else {
            dynlist_push(buffer, tok);
        }
    }
    if (buffer->len > 0) {
        dynlist_push(final, transform_proc_fncdef(treename, buffer));
        dynlist_clear(buffer);
    }
    dynlist_destroy(buffer);
    return smart_create_tokenl(Group, noloc, final);
}
static char transform_is_fb_depth_increaser(ParserToken* tok) {
    char* w = tok->data.word;
    if (!(strcmp(w, "if") && strcmp(w, "for") && strcmp(w, "while"))) {
        return 1;
    }
    return 0;
}
static char transform_is_infix_keyword(ParserToken* tok) {
    char* w = tok->data.word;
    if (!(strcmp(w, "as") && strcmp(w, "is"))) {
        return 1;
    }
    return 0;
}
static Token* transform_proc_expression(char*, DynList*);
static void dynlist_transfer(DynList* input, DynList* output) {
    uint l = input->len;
    uint fl = output->len + l;
    while (fl > output->cap) {
        dynlist_expand(output);
    }
    for (uint i = 0; i < l; i ++) {
        dynlist_push(output, input->ptr[i]);
    }
    dynlist_clear(input);
}
/*
ensures that all non-trivial parts of an expression (eg. function calls, the dot operator) are processed
returns a new dynamic list that can contain parser tokens of type CONTENT_PROC
*/
static DynList* transform_proc_ntriv(char* treename, DynList* tokens) {
    // for (int i = 0; i < tokens->len; i ++) {
    //     parsertoken_print(tokens->ptr[i]);
    // }
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* build = dynlist_create(pointereq, no_release);
    DynList* buffer = dynlist_create(pointereq, no_release);
    int part = 0;
    int pari = 0;
    int depth;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        // printf("LOOP ITER: %d\n", i);
        // parsertoken_print(tok);
        // for (int j = 0; j < final->len; j ++) {
        //     parsertoken_print(dynlist_get(final, j));
        // }
        // printf("\n");
        loop_head:
        // looking for complex parts
        if (part == 0) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth = 1;
                    if (build->len > 0) {
                        ParserToken* bt = dynlist_pop(build);
                        if (bt->type == CONTENT_WORD) {
                            dynlist_transfer(build, final);
                            dynlist_push(build, smart_create_tokent(Ident, bt, strmove(bt->data.word)));
                            part = 2;
                            continue;
                        }
                        dynlist_push(build, bt);
                        dynlist_transfer(build, final);
                    }
                    part = 3;
                    continue;
                }
                if (!strcmp(tok->data.symbol, ".")) {
                    part = 1;
                    pari = 0;
                    if (build->len > 0) {
                        ParserToken* ot = dynlist_pop(build);
                        dynlist_transfer(build, final);
                        dynlist_push(build, smart_create_tokent(Ident, ot, strmove(ot->data.word)));
                    }
                    dynlist_push(build, smart_create_tokenl(Operator, tokloc, '.'));
                    continue;
                }
            }
            dynlist_push(build, tok);
        } else if (part == 1) {
            if (pari == 0) {
                if (tok->type != CONTENT_WORD) {
                    transform_error("expected member", tok);
                }
                dynlist_push(build, smart_create_tokenl(Ident, tokloc, strmove(tok->data.word)));
                pari = 1;
            } else {
                pari = 0;
                if (tok->type == CONTENT_WORD) {
                    if (transform_is_infix_keyword(tok)) {
                        part = 0;
                        ParserData d = {.processed=dynlist_reown(build)};
                        dynlist_push(final, create_parsertoken(CONTENT_PROC, -1, -1, treename, d));
                        dynlist_push(final, tok);
                        continue;
                    }
                }
                if (tok->type != CONTENT_SYM) {
                    transform_error("expected operator or function call", tok);
                }
                if (!strcmp(tok->data.symbol, ".")) {
                    dynlist_push(build, smart_create_tokenl(Operator, tokloc, '.'));
                } else if (!strcmp(tok->data.symbol, "(")) {
                    Token* ot = dynlist_pop(build);
                    free(dynlist_pop(build));
                    Token* g = smart_create_tokenl(Group, noloc, dynlist_reown(build));
                    dynlist_push(build, ot);
                    dynlist_push(build, g);
                    depth = 1;
                    part = 2;
                } else {
                    part = 0;
                    ParserData d = {.processed=dynlist_reown(build)};
                    dynlist_push(final, create_parsertoken(CONTENT_PROC, -1, -1, treename, d));
                    dynlist_push(final, tok);
                }
            }
        } else if (part == 2) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth ++;
                } else if (!strcmp(tok->data.symbol, ")")) {
                    depth --;
                }
                if (depth == 0 || (depth == 1 && !strcmp(tok->data.symbol, ","))) {
                    if (buffer->len > 0) {
                        dynlist_push(build, transform_proc_expression(treename, buffer));
                    }
                    dynlist_clear(buffer);
                    if (depth == 0) {
                        DynList* dl = dynlist_create(pointereq, no_release);
                        dynlist_push(dl, smart_create_tokenl(Node, noloc, dynlist_reown(build)));
                        ParserData d = {.processed=dl};
                        dynlist_push(final, create_parsertoken(CONTENT_PROC, -1, -1, treename, d));
                        part = 0;
                    }
                    continue;
                }
            }
            dynlist_push(buffer, tok);
        } else if (part == 3) {
            if (tok->type == CONTENT_SYM) {
                if (!strcmp(tok->data.symbol, "(")) {
                    depth ++;
                } else if (!strcmp(tok->data.symbol, ")")) {
                    depth --;
                }
                if (depth == 0) {
                    dynlist_push(build, transform_proc_expression(treename, buffer));
                    dynlist_clear(buffer);
                    ParserData d = {.processed=dynlist_reown(build)};
                    dynlist_push(final, create_parsertoken(CONTENT_PROC, -1, -1, treename, d));
                    part = 0;
                    continue;
                }
            }
            dynlist_push(buffer, tok);
        }
    }
    if (build->len > 0) {
        if (((Token*)dynlist_get(build, 0))->chk == 1) {
            ParserData d = {.processed=dynlist_reown(build)};
            dynlist_push(final, create_parsertoken(CONTENT_PROC, -1, -1, treename, d));
        } else {
            dynlist_transfer(build, final);
        }
    }
    // for (int i = 0; i < final->len; i ++) {
    //     parsertoken_print(dynlist_get(final, i));
    // }
    dynlist_destroy(build);
    dynlist_destroy(buffer);
    return final;
}
// static Token* _convert_token(char* treename, ParserToken* tok, long line, const char* func, const char* file) {
static Token* convert_token(char* treename, ParserToken* tok) {
    if (tok->chk == 1) {
        transform_error("ATTEMPT TO DOUBLE CONVERT", &BADTOK);
    }
    // printf("%li (%s){%s}\n", line, func, file);
    // parsertoken_print(tok);
    if (tok->type == CONTENT_CHAR) {
        return smart_create_tokenl(Char, tokloc, tok->data.character);
    }
    if (tok->type == CONTENT_FLOT) {
        return smart_create_tokenl(Float, tokloc, tok->data.floating);
    }
    if (tok->type == CONTENT_INT) {
        return smart_create_tokenl(Int, tokloc, tok->data.integer);
    }
    if (tok->type == CONTENT_STR) {
        return smart_create_tokenl(String, tokloc, tok->data.string);
    }
    if (tok->type == CONTENT_WORD) {
        return smart_create_tokenl(Ident, tokloc, tok->data.word);
    }
    transform_error("unconvertable token", tok);
    return NULL;
}
// #define convert_token(treename, tok) _convert_token(treename, tok, __LINE__, __func__, __FILE__)
static char* keys[] = {"as", "is"};
static char* unary[] = {"~", "!", "++", "--", "!-!"/*internal representation of unary minus*/};
static char* expon[] = {"**"};
static char* muldiv[] = {"*", "/", "%"};
static char* addsub[] = {"+", "-"};
static char* bitwise[] = {"<<", ">>", "&", "|", "^"};
static char* comps[] = {"<=", ">=", "<", ">", "!=", "==", "&&", "||"};
static char** operators[] = {keys, unary, expon, muldiv, addsub, bitwise, comps};
static int lengths[] = {2, 5, 1, 3, 2, 5, 8};
static char determine_op_precedence(char* op1, char* op2) {
    int o1p = 0, o2p = 0;
    for (int i = 0; i < 7; i ++) {
        for (int j = 0; j < lengths[i]; j ++) {
            if (!strcmp(op1, operators[i][j])) {
                o1p = i;
            }
            if (!strcmp(op2, operators[i][j])) {
                o2p = i;
            }
        }
    }
    return o1p > o2p;
}
static char is_op_token(ParserToken* tok) {
    if (tok->type == CONTENT_SYM) {
        return 1;
    }
    if (tok->type == CONTENT_WORD) {
        if (transform_is_infix_keyword(tok)) {
            return 1;
        }
    }
    return 0;
}
static char determine_optok_precedence(ParserToken* op1, ParserToken* op2) {
    char *s1, *s2;
    if (op1->type == CONTENT_SYM) {
        s1 = op1->data.symbol;
    } else {
        s1 = op1->data.word;
    }
    if (op2->type == CONTENT_SYM) {
        s2 = op2->data.symbol;
    } else {
        s2 = op2->data.word;
    }
    return determine_op_precedence(s1, s2);
}
static char is_unary_op(ParserToken* tok) {
    char* s = tok->data.symbol;
    for (int i = 0; i < lengths[1]; i ++) {
        if (!strcmp(s, unary[i])) {
            return 1;
        }
    }
    return 0;
}
static int exprdepth = 0;
/*
processes an expression, cannot process assignment
*/
static Token* transform_proc_expression(char* treename, DynList* itoks) {
    exprdepth ++;
    // for (int i = 0; i < itoks->len; i ++) {
    //     parsertoken_print_condensed(itoks->ptr[i]);
    // }
    // printf("\n\n");
    DynList* tokens = transform_proc_ntriv(treename, itoks);
    // printf("NTRIV PROCD TOKS:\n");
    // for (int i = 0; i < tokens->len; i ++) {
    //     // parsertoken_print_condensed(tokens->ptr[i]);
    //     ParserToken* t = tokens->ptr[i];
    //     parsertoken_print(t);
    //     // if (t->type == CONTENT_INT) {
    //     //     print_semantic_token((Token*)t, "");
    //     // }
    // }
    // printf("\n");
    LinkedList* yardstack = LINKED_OBJECTS;
    LinkedList* yardstack2 = LINKED_OBJECTS;
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* build = dynlist_create(pointereq, no_release);
    int flag = 0;
    int f2 = 0;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        // printf("CHECKi: %d (%d)\n", tok->chk, i);
        // parsertoken_print(tok);
        if (flag > 0) {
            ParserToken* opt;
            if (f2 == 0) {
                if (is_op_token(tok)) {
                    goto igflag;
                }
                opt = linkedlist_pop(yardstack);
                if (!strcmp(opt->data.symbol, "!-!")) {
                    dynlist_push(build, smart_create_tokent(Ident, opt, strmove("-")));
                } else {
                    dynlist_push(build, smart_create_tokent(Ident, opt, strmove(opt->data.symbol)));
                }
            }
            if (tok->type == CONTENT_PROC) {
                // printf("PROCPRINT:\n");
                // parsertoken_print(tok);
                DynList* lst = tok->data.processed;
                // printf("PROCCAP: %d PROCLEN: %d\n", lst->cap, lst->len);
                // printf("PROCPTR: %p\n", (void*)lst->ptr);
                // for (int j = 0; j < lst->cap; j ++) {
                //     printf("(%p) ", lst->ptr[j]);
                // }
                // printf("\n");
                for (int j = 0; j < lst->len; j ++) {
                    linkedlist_push(yardstack2, lst->ptr[j]);
                }
                free(tok->data.processed->ptr);
                free(tok->data.processed);
                free(tok);
                // printf("PROC ADD\n");
                Token* rtok = linkedlist_remove_at(yardstack2, 0);
                dynlist_push(build, rtok);
                // printf("%p\n", (void*)rtok);
                // printf("YS2 STATE: size=%d head=%p tail=%p\n", yardstack2->size, (void*)yardstack2->head, (void*)yardstack2->tail);
            } else {
                dynlist_push(build, convert_token(treename, tok));
            }
            if (f2 == 0) {
                if (!is_unary_op(opt)) {
                    // printf("NON-UNARY ADD\n");
                    // printf("%d\n", yardstack2->size);
                    // dynlist_push(build, linkedlist_remove_at(yardstack2, 0));
                    // printf("%p\n", build->ptr[build->len-1]);
                    // printf("YS2 STATE: size=%d head=%p tail=%p\n", yardstack2->size, (void*)yardstack2->head, (void*)yardstack2->tail);
                    Token* rtok = linkedlist_remove_at(yardstack2, 0);
                    dynlist_push(build, rtok);
                    // printf("%p\n", (void*)rtok);
                    // printf("YS2 STATE: size=%d head=%p tail=%p\n", yardstack2->size, (void*)yardstack2->head, (void*)yardstack2->tail);
                }
                dynlist_push(final, smart_create_tokenl(Node, noloc, dynlist_reown(build)));
            } else {
                dynlist_push(final, smart_create_tokenl(Group, noloc, dynlist_reown(build)));
            }
            flag --;
            f2 = 0;
            continue;
        }
        igflag:
        if (is_op_token(tok)) {
            if (tok->type == CONTENT_WORD) {
                dynlist_push(build, linkedlist_remove_at(yardstack2, 0));
                Token* t;
                if (!strcmp(tok->data.word, "as")) {
                    t = smart_create_tokenl(Operator, tokloc, '*');
                } else if (!strcmp(tok->data.word, "is")) {
                    t = smart_create_tokenl(Keyword, tokloc, KEYWORD_IS);
                } else {
                    transform_error("unhandled infix keyword", tok);
                }
                dynlist_push(build, t);
                flag ++;
                f2 = 1;
                continue;
            }
            if (yardstack->size > 0) {
                char hasprec = determine_optok_precedence(tok, linkedlist_get(yardstack, 0)->data);
                ParserToken* opt;
                if (!hasprec) {
                    opt = linkedlist_pop(yardstack);
                    // dynlist_push(build, smart_create_tokent(Ident, opt, strmove(opt->data.symbol)));
                }
                linkedlist_push(yardstack, tok);
                if (!hasprec) {
                    linkedlist_push(yardstack, opt);
                }
                flag ++;
            } else {
                linkedlist_push(yardstack, tok);
                flag ++;
            }
            continue;
        }
        if (tok->type == CONTENT_PROC) {
            for (int j = 0; j < tok->data.processed->len; j ++) {
                linkedlist_push(yardstack2, tok->data.processed->ptr[i]);
            }
            free(tok->data.processed->ptr);
            free(tok->data.processed);
            free(tok);
        } else {
            linkedlist_push(yardstack2, convert_token(treename, tok));
        }
    }
    // printf("MBDONE\n");
    while (yardstack2->size > 0) {
        Token* pt = linkedlist_remove_at(yardstack2, 0);
        // printf("CHECK: %d\n", pt->chk);
        // if (pt->type == CONTENT_PROC) {
        //     dynlist_transfer(pt->data.processed, final);
        //     free(pt->data.processed->ptr);
        //     free(pt->data.processed);
        //     free(pt);
        //     continue;
        // }
        // dynlist_push(final, convert_token(treename, pt));
        dynlist_push(final, pt);
    }
    // printf("FTDONE\n");
    dynlist_destroy(tokens);
    dynlist_destroy(build);
    linkedlist_destroy(yardstack);
    linkedlist_destroy(yardstack2);
    exprdepth --;
    // printf("DEPTH: %d\n", exprdepth);
    // for (int i = 0; i < final->len; i ++) {
    //     printf("(%p) ", final->ptr[i]);
    // }
    // printf("\n");
    return smart_create_tokenl(Group, noloc, final);
}
// static Token* transform_proc_condition(char* treename, DynList* tokens) {
//     DynList* final = dynlist_create(pointereq, no_release);
//     int part = 0;
//     for (int i = 0; i < tokens->len; i ++) {
//         ParserToken* tok = (ParserToken*)tokens->ptr[i];
//         if (part == 0) {}
//     }
//     return smart_create_tokenl(Group, noloc, final);
// }
static char is_assignment(ParserToken* tok) {
    char* s = tok->data.symbol;
    if (!(strcmp(s, ">=") && strcmp(s, "<=") && strcmp(s, "==") && strcmp(s, "!="))) {
        return 0;
    }
    char c;
    while ((c=*s++)) {
        if (c == '=') return 1;
    }
    return 0;
}
static Token* clone_name(Token* name) {
    DynList* intern = dynlist_create(pointereq, no_release);
    for (int i = 0; i < name->data.group->len; i ++) {
        Token* p = dynlist_get(name->data.group, i);
        Token* t;
        if (p->type == Ident) {
            TokenData d = {.identifier=strmove(p->data.identifier)};
            t = token_create(Ident, p->line, p->column, p->file, d);
        } else {
            TokenData d = {.operator='.'};
            t = token_create(Operator, p->line, p->column, p->file, d);
        }
        dynlist_push(intern, t);
    }
    TokenData d = {.group=intern};
    return token_create(Group, name->line, name->column, name->file, d);
}
static Token* transform_proc_fncbod(char* treename, DynList* tokens) {
    DynList* final = dynlist_create(pointereq, no_release);
    DynList* buffer = dynlist_create(pointereq, no_release);
    DynList* build = dynlist_create(pointereq, no_release);
    int part = 0;
    int kind = 0;
    int s, e, depth;
    for (int i = 0; i < tokens->len; i ++) {
        ParserToken* tok = (ParserToken*)tokens->ptr[i];
        if (part == 0) {
            if (tok->type == CONTENT_LINE) continue;
            if (tok->type == CONTENT_WORD) {
                if (!strcmp(tok->data.word, "if")) {
                    kind = 0;
                    part = 2;
                    dynlist_push(build, smart_create_tokenl(Keyword, tokloc, KEYWORD_IF));
                } else if (!strcmp(tok->data.word, "else")) {
                    kind = 1;
                } else if (!strcmp(tok->data.word, "while")) {
                    kind = 2;
                } else if (!strcmp(tok->data.word, "do")) {
                    //
                } else if (!strcmp(tok->data.word, "for")) {
                    kind = 3;
                } else {
                    dynlist_push(buffer, tok);
                    part = 1;
                }
            } else {
                transform_error("expected word", tok);
            }
        } else if (part == 1) {
            if (tok->type == CONTENT_LINE) {
                int str = -1;
                char simpl = 1;
                DynList bufferwin = {.eq=pointereq,.rel=no_release,.cap=0,.len=buffer->len,.ptr=buffer->ptr};
                for (int j = 0; j < buffer->len; j ++) {
                    ParserToken* pt = dynlist_get(buffer, j);
                    if (pt->type == CONTENT_SYM) {
                        if (is_assignment(pt)) {
                            simpl = 0;
                            str = j;
                            if (!strcmp(pt->data.symbol, "=")) {
                                simpl = 1;
                            }
                            break;
                        }
                    }
                }
                if (str != -1) {
                    bufferwin.len -= str + 1;
                    bufferwin.ptr += str + 1;
                    DynList* minibuf = dynlist_create(pointereq, no_release);
                    int pari = 0;
                    for (int j = 0; j < str; j ++) {
                        ParserToken* pt = dynlist_get(buffer, j);
                        if (pt->type == CONTENT_WORD) {
                            if (pari == 0) {
                                dynlist_push(minibuf, convert_token(treename, pt));
                            } else {
                                dynlist_push(minibuf, pt);
                            }
                        } else if (pt->type == CONTENT_SYM) {
                            if (!strcmp(pt->data.symbol, ".")) {
                                dynlist_push(minibuf, smart_create_tokent(Operator, pt, '.'));
                            } else if (!strcmp(pt->data.symbol, ":")) {
                                dynlist_push(build, smart_create_tokenl(Group, noloc, dynlist_reown(minibuf)));
                                dynlist_push(build, smart_create_tokent(Operator, pt, ':'));
                                pari = 1;
                            }
                        }
                    }
                    if (pari == 1) {
                        dynlist_push(build, transform_proc_generic(treename, minibuf));
                    } else {
                        dynlist_push(build, smart_create_tokenl(Group, noloc, dynlist_reown(minibuf)));
                    }
                    dynlist_destroy(minibuf);
                    dynlist_push(build, smart_create_tokent(Operator, dynlist_get(buffer, str), '='));
                    if (!simpl) {
                        DynList* tmp = dynlist_create(pointereq, no_release);
                        ParserToken* op = dynlist_get(buffer, str);
                        char* s = calloc(strlen(op->data.symbol), 1);
                        for (int j = 0; j < strlen(op->data.symbol)-1; j ++) {
                            s[j] = op->data.symbol[j];
                        }
                        dynlist_push(tmp, smart_create_tokent(Ident, op, s));
                        dynlist_push(tmp, clone_name(dynlist_get(build, 0)));
                        dynlist_push(build, smart_create_tokenl(Node, noloc, tmp));
                    }
                } else {
                    ParserToken* ft = dynlist_get(buffer, 0);
                    if (ft->type == CONTENT_WORD) {
                        if (!strcmp(ft->data.word, "return")) {
                            dynlist_push(build, smart_create_tokent(Keyword, ft, KEYWORD_RETURN));
                            bufferwin.len -= 1;
                            bufferwin.ptr += 1;
                        }
                    }
                }
                // for (int j = 0; j < bufferwin.len; j ++) {
                //     parsertoken_print_condensed(dynlist_get(&bufferwin, j));
                // }
                // printf("\n");
                Token* t = transform_proc_expression(treename, &bufferwin);
                // printf("DONE\n");
                if (str >= 0 && !simpl) {
                    dynlist_push(((Token*)build->ptr[build->len-1])->data.node, t);
                } else {
                    dynlist_push(build, t);
                }
                dynlist_push(final, smart_create_tokenl(Node, noloc, dynlist_reown(build)));
                dynlist_clear(buffer);
                part = 0;
                continue;
            }
            dynlist_push(buffer, tok);
        } else if (part == 2) {
            if (tok->type == CONTENT_LINE) {
                // dynlist_push(build, transform_proc_condition(treename, buffer));
                dynlist_clear(buffer);
                part = 3;
                s = i + 1;
            } else {
                dynlist_push(buffer, tok);
            }
        } else if (part == 3) {
            if (tok->type == CONTENT_WORD) {
                if (transform_is_fb_depth_increaser(tok)) {
                    depth ++;
                } else if (!strcmp(tok->data.word, "end")) {
                    depth --;
                }
                if (depth == 0) {
                    e = i;
                    if (s != e) {
                        DynList window = {.eq=pointereq,.rel=no_release,.cap=0,.len=(e-s),.ptr=tokens->ptr};
                        dynlist_push(build, transform_proc_fncbod(treename, &window));
                    } else {
                        dynlist_push(build, smart_create_tokenl(Group, noloc, dynlist_create(pointereq, no_release)));
                    }
                    dynlist_push(final, smart_create_tokenl(Node, noloc, dynlist_reown(build)));
                }
            }
        }
    }
    return smart_create_tokenl(Group, noloc, final);
}

static Token* transform_parser_tree(char* treename, DynList* partree, LinkedList* tofollow) {
    Token* semroot = smart_create_token(Nmsp, -1, -1, treename, treename);
    LinkedList* yardstack = LINKED_OBJECTS;
    LinkedList* inclusions = LINKED_OBJECTS;
    DynList* buildnode = dynlist_create(pointereq, no_release);
    int sourcephase = PHASE_INCL;
    int subsec = 0;
    int stmtkind = 0;
    int typetype = 0;
    int depth = 0;
    DynList* tokbufx = dynlist_create(pointereq, no_release);
    DynList* tokbufy = dynlist_create(pointereq, no_release);
    ParserToken *held;
    // for (int i = 0; i < partree->len; i ++) printf("%d:%p\n", i, partree->ptr[i]);
    // printf("\n");
    for (int im = 0; im < partree->len; im ++) {
        ParserToken* tok = (ParserToken*)partree->ptr[im];
        // parsertoken_print_condensed(tok);printf("\n");
        // for (int j = 0; j < partree->len; j ++) {
        //     printf("%p ", partree->ptr[j]);
        // }
        // printf("\n");
        if (subsec == 0) {
            if (tok->type == CONTENT_LINE) continue;
            if (tok->type != CONTENT_WORD) {
                transform_error("expected word", tok);
            }
        }
        inner_continue:
        if (sourcephase == PHASE_INCL) {
            if (subsec == SEC_INCL_NONE) {
                // if (tok->type == CONTENT_LINE) continue;
                // if (tok->type != CONTENT_WORD) {
                //     transform_error("expected word", tok);
                // }
                if (strcmp(tok->data.word, "use")) {
                    // printf("\nNOT USE, REPHASE\n");
                    sourcephase = rephase(sourcephase, tok);
                    // printf("PHASE=%d\n\n", sourcephase);
                    subsec = 0;
                    goto inner_continue;
                }
                subsec = SEC_INCL_USED;
                dynlist_push(buildnode, smart_create_tokenl(Keyword, tokloc, KEYWORD_USE));
                dynlist_clear(tokbufx);
                dynlist_clear(tokbufy);
            } else if (subsec == SEC_INCL_USED) {
                if (tok->type == CONTENT_WORD && !strcmp(tok->data.word, "of")) {
                    // printf("\n");
                    subsec = SEC_INCL_NAME;
                    held = tok;
                    goto loop_continue;
                }
                // printf("BXP; %p\n", (void*) tok);
                dynlist_push(tokbufx, tok);
            } else {
                if (tok->type == CONTENT_LINE) {
                    // printf("BX@ %p:%p\nBY@ %p:%p\n", (void*)tokbufx, (void*)tokbufx->ptr, (void*)tokbufy, (void*)tokbufy->ptr);
                    subsec = SEC_INCL_NONE;
                    DynList* btmp = dynlist_create(pointereq, no_release);
                    DynList* ntmp = dynlist_create(pointereq, no_release);
                    // printf("CAP: %d\n", tokbufx->cap);
                    // for (int i = 0; i < tokbufx->cap; i ++) printf("%d:%p\n", i, tokbufx->ptr[i]);
                    // printf("\n");
                    for (int i = 0; i < tokbufx->len; i ++) {
                        ParserToken* pt = (ParserToken*)tokbufx->ptr[i];
                        // printf("BP1 [%d]\n", i);
                        // parsertoken_print_condensed(pt);printf("\n");
                        // printf("AP1\n");
                        if (pt->type == CONTENT_SYM && !strcmp(pt->data.symbol, ",")) {
                            dynlist_push(btmp, smart_create_tokenl(Node, noloc, dynlist_reown(ntmp)));
                        } else if (pt->type != CONTENT_WORD && !(pt->type == CONTENT_SYM && (!strcmp(pt->data.symbol, ".") || !strcmp(pt->data.symbol, "*")))) {
                            transform_error("invalid use token", pt);
                        } else {
                            Token* ct;
                            if (pt->type == CONTENT_WORD) {
                                ct = smart_create_tokent(Ident, pt, strmove(pt->data.word));
                            } else {
                                ct = smart_create_tokent(Ident, pt, strmove("*"));
                            }
                            dynlist_push(ntmp, ct);
                        }
                    }
                    dynlist_push(btmp, smart_create_tokenl(Node, noloc, dynlist_reown(ntmp)));
                    dynlist_push(buildnode, smart_create_tokenl(Node, noloc, btmp));
                    dynlist_push(buildnode, smart_create_tokent(Keyword, held, KEYWORD_OF));
                    // printf("CAP: %d\n", tokbufy->cap);
                    // printf("LEN: %d\n", tokbufy->len);
                    // for (int i = 0; i < tokbufy->cap; i ++) printf("%d:%p\n", i, tokbufy->ptr[i]);
                    // printf("\n");
                    for (int i = 0; i < tokbufy->len; i ++) {
                        ParserToken* pt = (ParserToken*)tokbufy->ptr[i];
                        // printf("BP2 [%d]\n", i);
                        // parsertoken_print_condensed(pt);printf("\n");
                        // printf("AP2\n");
                        if (pt->type != CONTENT_WORD && !(pt->type == CONTENT_SYM && !strcmp(pt->data.symbol, "."))) {
                            transform_error("invalid use token", pt);
                        } else {
                            Token* ct;
                            if (pt->type == CONTENT_WORD) {
                                ct = smart_create_tokent(Ident, pt, strmove(pt->data.word));
                            } else {
                                ct = smart_create_tokent(Operator, pt, '.');
                            }
                            dynlist_push(ntmp, ct);
                        }
                    }
                    dynlist_push(buildnode, smart_create_tokenl(Node, noloc, ntmp));
                    dynlist_push(semroot->data.namespace.childnode, smart_create_tokenl(Node, noloc, dynlist_reown(buildnode)));
                    dynlist_clear(tokbufx);
                    dynlist_clear(tokbufy);
                    goto loop_continue;
                }
                // printf("BYP; %p\n", (void*)tok);
                dynlist_push(tokbufy, tok);
            }
        } else if (sourcephase == PHASE_GLBL) {
            todo();
        } else if (sourcephase == PHASE_TYPE) {
            if (subsec == 0) { // |typedef or |pattern
                if (strcmp(tok->data.word, "typedef") && strcmp(tok->data.word, "pattern")) {
                    sourcephase = rephase(sourcephase, tok);
                    subsec = 0;
                    stmtkind = 0;
                    goto inner_continue;
                }
                if (strcmp(tok->data.word, "pattern")) {
                    dynlist_push(buildnode, smart_create_tokenl(Keyword, tokloc, KEYWORD_TYPEDEF));
                } else {
                    stmtkind = 1;
                    dynlist_push(buildnode, smart_create_tokenl(Keyword, tokloc, KEYWORD_PATTERN));
                }
                subsec = SEC_TYPE_NAME;
                if (!(im < partree->len-1)) {
                    transform_error("incomplete definition", tok);
                }
            } else if (subsec == SEC_TYPE_NAME) { // typdef |name or pattern |name
                if (tok->type == CONTENT_WORD) {
                    if (!strcmp(tok->data.word, "is")) {
                        dynlist_push(buildnode, transform_proc_generic(treename, tokbufx));
                        dynlist_clear(tokbufx);
                        subsec = SEC_TYPE_SPEC;
                        continue;
                    }
                }
                if (tok->type == CONTENT_LINE) {
                    transform_error("expected token", tok);
                }
                dynlist_push(tokbufx, tok);
            } else if (subsec == SEC_TYPE_SPEC) { // typedef name of generic params is |
                if (tok->type == CONTENT_LINE) {
                    Token* g = smart_create_tokenl(Group, noloc, dynlist_create(pointereq, no_release));
                    for (int i = 0; i < tokbufy->len; i ++) {
                        ParserToken* t = (ParserToken*)tokbufy->ptr[i];
                        dynlist_push(g->data.group, smart_create_tokent(Ident, t, strmove(t->data.word)));
                    }
                    dynlist_clear(tokbufy);
                    dynlist_push(buildnode, g);
                    if (typetype) {
                        dynlist_push(buildnode, smart_create_tokenl(Node, noloc, dynlist_create(pointereq, no_release)));
                        dynlist_push(((Token*)buildnode->ptr[buildnode->len-1])->data.node, smart_create_tokent(Keyword, held, (typetype == 1)?KEYWORD_SUM:KEYWORD_PROD));
                    }
                    subsec = (stmtkind == 0) ? SEC_TYPE_BODY : SEC_PATT_BODY;
                    continue;
                }
                if (tok->type != CONTENT_WORD) {
                    transform_error("expected word", tok);
                }
                if (stmtkind) { // pattern
                    if (stmtkind & 2) {
                        if (strcmp(tok->data.word, "and")) {
                            transform_error("invalid super-pattern specifier", tok);
                        }
                        stmtkind &= 1;
                    } else {
                        stmtkind |= 2;
                        dynlist_push(tokbufy, tok);
                    }
                } else {
                    if (!typetype) {
                        if (strcmp(tok->data.word, "sum") && strcmp(tok->data.word, "prod")) {
                            dynlist_push(tokbufy, tok);
                            subsec = SEC_TYPE_INLN;
                            continue;
                        } else {
                            held = tok;
                            if (!strcmp(tok->data.word, "sum")) {
                                typetype = 1;
                            } else {
                                typetype = 2;
                            }
                        }
                    } else {
                        dynlist_push(tokbufy, tok);
                    }
                }
            } else if (subsec == SEC_TYPE_INLN) { // typedef name of generic params is [something]|
                if (tok->type == CONTENT_LINE) {
                    char isinlinesum = 0;
                    for (int i = 0; i < tokbufy->len; i ++) {
                        ParserToken* ctok = (ParserToken*)tokbufy->ptr[i];
                        if (ctok->type == CONTENT_SYM && !strcmp(ctok->data.symbol, "|")) {
                            isinlinesum = 1;
                        }
                    }
                    if (isinlinesum) {
                        for (int i = 0; i < tokbufy->len; i ++) {
                            ParserToken* ctok = (ParserToken*)tokbufy->ptr[i];
                            if (ctok->type == CONTENT_SYM && !strcmp(ctok->data.symbol, "|")) {
                                ParserData d = {.line='\n'};
                                dynlist_push(tokbufx, create_parsertoken(CONTENT_LINE, ctok->line, -1, treename, d));
                            } else {
                                dynlist_push(tokbufx, ctok);
                            }
                        }
                        DynList* lst = dynlist_create(pointereq, no_release);
                        dynlist_push(lst, smart_create_token(Keyword, ((ParserToken*)tokbufy->ptr[0])->line, -1, treename, KEYWORD_SUM));
                        dynlist_push(lst, transform_proc_defbod(treename, tokbufx, 1));
                        dynlist_push(buildnode, smart_create_tokenl(Node, noloc, lst));
                        dynlist_clear(tokbufy);
                        dynlist_clear(tokbufx);
                        subsec = SEC_TYPE_NONE;
                        dynlist_push(semroot->data.namespace.childnode, smart_create_tokenl(Node, noloc, dynlist_reown(buildnode)));
                        continue;
                    } else {
                        dynlist_push(buildnode, transform_proc_generic(treename, tokbufy));
                        dynlist_push(semroot->data.namespace.childnode, smart_create_tokenl(Node, noloc, dynlist_reown(buildnode)));
                        dynlist_clear(tokbufx);
                        dynlist_clear(tokbufy);
                        subsec = SEC_TYPE_NONE;
                    }
                } else {
                    dynlist_push(tokbufy, tok);
                }
            } else {
                if (tok->type == CONTENT_WORD) {
                    if (!strcmp(tok->data.word, "end")) {
                        DynList* pushto = ((Token*)buildnode->ptr[buildnode->len-1])->data.node;
                        if (stmtkind == 0) {
                            dynlist_push(pushto, transform_proc_defbod(treename, tokbufy, typetype));
                        } else {
                            dynlist_push(buildnode, transform_proc_patbod(treename, tokbufy));
                        }
                        typetype = 0;
                        dynlist_clear(tokbufy);
                        subsec = SEC_TYPE_NONE;
                        dynlist_push(semroot->data.namespace.childnode, smart_create_tokenl(Node, noloc, dynlist_reown(buildnode)));
                        continue;
                    }
                }
                dynlist_push(tokbufy, tok);
            }
        } else if (sourcephase == PHASE_FUNC) {
            if (subsec == 0) {
                dynlist_push(tokbufx, tok);
                subsec = 1;
            } else if (subsec == 1) {
                if (tok->type == CONTENT_LINE) {
                    dynlist_push(buildnode, transform_proc_fncdef(treename, tokbufx));
                    dynlist_clear(tokbufx);
                    depth = 1;
                    subsec = 2;
                } else {
                    dynlist_push(tokbufx, tok);
                }
            } else if (subsec == 2) {
                if (tok->type == CONTENT_WORD) {
                    if (transform_is_fb_depth_increaser(tok)) {
                        depth ++;
                    } else if (!strcmp(tok->data.word, "end")) {
                        depth --;
                    }
                    if (depth == 0) {
                        dynlist_push(buildnode, transform_proc_fncbod(treename, tokbufx));
                        dynlist_clear(tokbufx);
                        dynlist_push(semroot->data.namespace.childnode, smart_create_tokenl(Node, noloc, dynlist_reown(buildnode)));
                        subsec = 0;
                        continue;
                    }
                }
                dynlist_push(tokbufx, tok);
            }
        }
        loop_continue:
        continue;
    }
    return semroot;
}

#undef tokloc
#undef noloc

#undef PHASE_INCL
#undef PHASE_GLBL
#undef PHASE_TYPE
#undef PHASE_FUNC
#undef smart_create_token
#undef smart_create_tokenl
#undef smart_create_tokent

/*
mode can be 0 or 1
if mode is 1, inclusions will not be processed and a LinkedList of ParserTokens is returned
*/
Token* parse_file(char* fpath, char mode, LinkedList* tofollow) {
    parser_init();
    if (check_file(fpath)) { // don't process the same file multiple times
        return NULL;
    }
    FILE* fp;
    if ((fp=fopen(fpath, "r")) == NULL) {
        printf("ERROR\n");
        goto bottom;
    }
    LinkedList* tokens = LINKED_OBJECTS; // the current statements's tokens
    StrBuf* buf = strbuf_create();
    strbuf_push(buf, '\x1b');
    int content = CONTENT_NONE; // type of content within buf
    int readch; // the result of getc
    char escape = 0; // if in an escape sequence
    char lineend = 0; // if a newline was encountered outside of a string
    char canintbase = 0; // if there can be a change in base
    char comment = 0; // if the parser is in a line comment
    char blockcomment = 0; // if the parser is in a block comment
    char intbase = BASE_10;
    char nlproc = 0;
    // line and column info
    long line = 1, col = 0;
    // the line and column when the token was started
    long tline = 1, tcol = 1;
    long nlcol = 1;
    Token* fintree;
    #define location tline, tcol, fpath
    #define clocation line, col, fpath
    char last = 0;
    char c = 0;
    datastruct_debug_disable_mask(DATASTRUCTDBG_LINKEDLIST);
    loop_continue:
    while ((readch = getc(fp)) != EOF) {
        // line and column updates
        col ++;
        if (nlproc) {
            ParserData d = {.line='\n'};
            linkedlist_push(tokens, create_parsertoken(CONTENT_LINE, line, col-1, fpath, d));
            nlproc = 0;
            col = 1;
            line ++;
        }
        if (readch == '\n') {
            nlcol = col;
            nlproc = 1;
        }
        last = c;
        c = (char)readch;
        if (comment) {
            if (c == '\n') {
                comment = 0;
            }
            continue;
        }
        if (blockcomment) {
            if (c == '/' && last == '*') {
                blockcomment = 0;
                strbuf_discard(buf);
                content = CONTENT_NONE;
            }
            continue;
        }
        inner_continue:
        if (last == '/') {
            if (c == '/') {
                comment = 1;
                content = CONTENT_NONE;
                continue;
            } else if (c == '*') {
                blockcomment = 1;
                content = CONTENT_NONE;
                continue;
            }
        }
        switch (content) {
            case CONTENT_NONE: // the buffer contains either meaningless nonsense or nothing at all
                // update token source locations
                tline = line;
                tcol = col;
                // discard any garbage that might be present
                strbuf_discard(buf);
                // figure out what kind of token the character belongs to
                content = reval_content(c);
                // don't put opening quotes into the buffer
                if (CONTENT_NOTSTR(content)) strbuf_push(buf, c);
                // set the flag that allows changing the int base if the int starts with a zero
                if (content == CONTENT_INT) {
                    if (c == '0') canintbase = 1;
                }
                break;
            case CONTENT_WORD: // alphanumeric + _
                if (!IS_WORDC(c)) {
                    ParserData d = {.word=strbuf_consume(buf)};
                    linkedlist_push(tokens, create_parsertoken(CONTENT_WORD, location, d));
                    content = CONTENT_NONE;
                    goto inner_continue;
                }
                strbuf_push(buf, c);
                break;
            case CONTENT_SYM: // characters that aren't valid in words but are also not control characters
                if (IS_WORDC(c) || check_symbol_continuity(buf, c) == 0) { // >> is one symbol, but >! is two
                    ParserData d = {.symbol=strbuf_consume(buf)};
                    linkedlist_push(tokens, create_parsertoken(CONTENT_SYM, location, d));
                    content = CONTENT_NONE;
                    goto inner_continue;
                }
                strbuf_push(buf, c);
                break;
            case CONTENT_INT:
                if (canintbase) { // change of base is allowed
                    canintbase = 0;
                    if (IS_ALPHA(c)) { // alternate base is being used
                        switch (c){
                            case'b':intbase=BASE_2;break;
                            case'o':intbase=BASE_8;break;
                            case'x':intbase=BASE_16;break;
                            default:parsing_error("invalid base specifier", clocation);
                        }
                        strbuf_push(buf, c);
                        goto loop_continue;
                    }
                }
                if (!IS_ALNUM(c)) { // end of int
                    if (c == '.') { // attempt at float
                        if (intbase != BASE_10) { // floats can only be base 10
                            parsing_error("floats must be base 10", clocation);
                        }
                        content = CONTENT_FLOT;
                        strbuf_push(buf, c);
                        goto loop_continue;
                    } else { // parse the int
                        char* p = strbuf_consume(buf);
                        ParserData d = {.integer=parse_intstr(p)};
                        free(p);
                        linkedlist_push(tokens, create_parsertoken(CONTENT_INT, location, d));
                        content = CONTENT_NONE;
                        goto inner_continue;
                    }
                }
                // validate that random garbage isn't in the number
                if (intbase == BASE_2 && (c<'0'||c>'1')) {
                    parsing_error("invalid integer literal", clocation);
                } else if (intbase == BASE_8 && (c<'0'||c>'7')) {
                    parsing_error("invalid integer literal", clocation);
                } else if (intbase == BASE_10 && (c<'0'||c>'9')) {
                    parsing_error("invalid integer literal", clocation);
                } else if (intbase == BASE_16 && ((c<'0'||c>'9')&&!(c>='a'&&c<='f'))) {
                    parsing_error("invalid integer literal", clocation);
                }
                strbuf_push(buf, c);
                break;
            case CONTENT_FLOT:
                if (IS_SYMBL(c) || c == '\n') { // parse the float
                    char* p = strbuf_consume(buf);
                    ParserData d = {.floating=parse_floatstr(p)};
                    free(p);
                    linkedlist_push(tokens, create_parsertoken(CONTENT_FLOT, location, d));
                    content = CONTENT_NONE;
                    goto inner_continue;
                }
                if (!IS_NUMER(c)) {
                    printf("'%s' + '%c'\n", buf->ptr, c);
                    parsing_error("invalid float literal", clocation);
                }
                strbuf_push(buf, c);
                break;
            case CONTENT_CHAR: // any single character
                if (escape) { // force the escaped character to be added
                    escape = 0;
                } else {
                    if (c == '\\') { // '\'' '\x1b' etc.
                        escape = 1;
                    }
                    if (c == '\'') {
                        if (DEBUG_ESCAPES) printf("\n(%li, %li, %s)\n", tline, tcol, fpath);
                        char* p = strbuf_consume(buf);
                        int resolved = resolve_charstr(p); // could be multi-byte code point
                        free(p);
                        ParserData d = {.character=resolved};
                        linkedlist_push(tokens, create_parsertoken(CONTENT_CHAR, location, d));
                        content = CONTENT_NONE;
                        goto loop_continue;
                    }
                }
                strbuf_push(buf, c);
                break;
            case CONTENT_STR: // like a char, but allows newlines in source and uses double quotes
                if (escape) {
                    escape = 0;
                } else {
                    if (c == '\\') {
                        escape = 1;
                    }
                    if (c == '"') {
                        if (DEBUG_ESCAPES) printf("\n(%li, %li, %s)\n", tline, tcol, fpath);
                        char* p = strbuf_consume(buf);
                        ParserData d = {.string=resolve_strstr(p)};
                        free(p);
                        linkedlist_push(tokens, create_parsertoken(CONTENT_STR, location, d));
                        content = CONTENT_NONE;
                        goto loop_continue;
                    }
                }
                strbuf_push(buf, c);
                break;
            default:
                content = CONTENT_NONE;
                break;
        }
    }
    #undef location
    #undef clocation
    loop_break:
    datastruct_debug_enable_mask(DATASTRUCTDBG_LINKEDLIST);
    {ParserData d = {.line='\n'};
    linkedlist_push(tokens, create_parsertoken(CONTENT_LINE, line, col-1, fpath, d));}
    // now transform the parser tokens into semantic tokens
    if (!mode) {
        DynList* dyntoks = dynlist_create(pointereq, no_release);
        LinkedListNode* c = tokens->head;
        while (c) {
            dynlist_push(dyntoks, c->data);
            c = c->next;
        }
        fintree = transform_parser_tree(fpath, dyntoks, tofollow);
        dynlist_destroy(dyntoks);
    }
    release:
    strbuf_destroy(buf);
    if (!mode) {
        linkedlist_destroy(tokens);
    }
    closefp:
    fclose(fp);
    if (mode) {
        printf("WARNING, DEBUG CODE RUNNING, DO NOT ATTEMPT ANYTHING BUT PARSER UNIT TESTING\n");
        return (Token*)tokens;
    }
    bottom:
    return fintree;
}

#undef CONTENT_NONE
#undef CONTENT_STR
#undef CONTENT_WORD
#undef CONTENT_INT
#undef CONTENT_FLOT
#undef CONTENT_SYM
#undef CONTENT_CHAR
#undef CONTENT_LINE
#undef CONTENT_NOTSTR
#undef BASE_2
#undef BASE_8
#undef BASE_10
#undef BASE_16

void parser_test_ptokens(char* fpath) {
    LinkedList* tofollow = LINKED_OBJECTS;
    LinkedList* list = (LinkedList*)parse_file(fpath, 1, tofollow);
    linkedlist_destroy(tofollow);
    printf("PARSED\n");
    LinkedListNode* curr = list->head;
    int cnt = 0;
    #define width 2
    while (curr) {
        parsertoken_print_condensed((ParserToken*)curr->data);
        if (cnt == (width-1)) printf("\n");
        cnt ++;
        cnt = cnt % width;
        curr = curr->next;
    }
    #undef width
    if (cnt) printf("\n");
    linkedlist_destroy(list);
}
void print_semantic_token(Token* tok, char* indent) {
    if (tok->chk == 2) {
        printf("ATTEMPT TO PRINT PARSER TOKEN\n");
        exit(1);
    }
    char* nide = strjoin(indent, " ");
    #define printh(n) printf("%s%s{%li:%li}", indent, n, tok->line, tok->column)
    printh(TYPENAMEMAP[tok->type]);
    switch (tok->type) {
        case Node:
            // printh("Node");
            printf("[\n");
            for (int i = 0; i < tok->data.node->len; i ++) {
                print_semantic_token((Token*)tok->data.node->ptr[i], nide);
            }
            printf("%s]\n", indent);
            break;
        case Nmsp:
            // printh("Nmsp");
            printf("('%s')[\n", tok->data.namespace.name);
            for (int i = 0; i < tok->data.namespace.childnode->len; i ++) {
                print_semantic_token((Token*)tok->data.namespace.childnode->ptr[i], nide);
            }
            printf("%s]\n", indent);
            break;
        case Generic:
            printf("(name='%s')[\n", tok->data.generic.name);
            for (int i = 0; i < tok->data.generic.restrictions->len; i ++) {
                print_semantic_token((Token*)tok->data.generic.restrictions->ptr[i], nide);
            }
            printf("%s]\n", indent);
            break;
        case Ident:printf("('%s')\n", tok->data.identifier);break;
        case Bool:printf("(%d)\n", tok->data.boolean);break;
        case Int:printf("(%li)\n", tok->data.integer);break;
        case Float:printf("(%f)\n", tok->data.floating);break;
        case Keyword:printf("(%s)\n", KEYWORDMAP[tok->data.keyword]);break;
        case Operator:printf("(%c)\n", tok->data.operator);break;
        case Char:
            printf("(");
            for (int i = 0; i < 4; i ++) {
                int v = (tok->data.character>>((3-i)*8))&0xff;
                if (v != 0 || i == 3) {
                    printf("%c", (char)v);
                }
            }
            printf(")\n");
            break;
        case String:printf("(%s)\n", tok->data.string);break;
        case Group:
            printf("[\n");
            for (int i = 0; i < tok->data.node->len; i ++) {
                print_semantic_token((Token*)tok->data.node->ptr[i], nide);
            }
            printf("%s]\n", indent);
            break;
        case Sep:printf("\n");break;
        case Type:printf("(%s)\n", tok->data.type);break;
        case Mod:printf("(%s)\n", MODIFMAP[tok->data.modifier]);break;
        default:break;
    }
    free(nide);
}
void parser_test_stokens(char* fpath) {
    LinkedList* tofollow = LINKED_OBJECTS;
    Token* output = parse_file(fpath, 0, tofollow);
    linkedlist_destroy(tofollow);
    printf("PARSED\n");
    print_semantic_token(output, "");
}

DynList* parser_parse(char* filename) {
    DynList* list = dynlist_create(pointereq, no_release);
    LinkedList* tofollow = LINKED_OBJECTS;
    linkedlist_push(tofollow, filename);
    while (tofollow->size > 0) {
        Token* ns = parse_file((char*)linkedlist_pop(tofollow), 0, tofollow);
        if (ns != NULL) {
            dynlist_push(list, ns);
        }
    }
    linkedlist_destroy(tofollow);
    return list;
}

#endif
