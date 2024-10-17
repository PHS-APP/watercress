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

char DEBUG_ESCAPES = 0;

static char inited = 0;
// list of processed files
static LinkedList* processed = NULL;
// the AST
static LinkedList* program = NULL;

static int inoeq(void* p1, void* p2) {
    return *((ino_t*)p1) == *((ino_t*)p2);
}

void parser_init(void) {
    if (inited) return;
    inited = 1;
    processed = linkedlist_create((ItemEq)inoeq);
    program = LINKED_OBJECTS;
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

static char* CONTENTTYPEMAP[] = {"NONE", "STR", "WORD", "INT", "FLOT", "SYM", "CHAR", "LINE"};

#define CONTENT_NONE 0
#define CONTENT_STR  1
#define CONTENT_WORD 2
#define CONTENT_INT  3
#define CONTENT_FLOT 4
#define CONTENT_SYM  5
#define CONTENT_CHAR 6
#define CONTENT_LINE 7

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
    // todo();
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
    // todo();
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
                    // long v = parse_hex(b2->ptr);
                    // strbuf_discard(b2);
                    // if (esc == 2) {
                    //     strbuf_push(buf, (char)v);
                    // } else {
                    //     long c1 = v >> 16, c2 = v & 0xffff;
                    //     if (c1 == 0) {
                    //         if ((c2 & ~0x7f) == 0) {
                    //             strbuf_push(buf, (char)(c2&0xff));
                    //         } else {
                    //             if ((c2 & ~0x7ff) == 0) {
                    //                 strbuf_push(buf, (char)(0xc0|(c2>>6)));
                    //                 strbuf_push(buf, (char)(0x80|(c2&0x3f)));
                    //             } else {
                    //                 strbuf_push(buf, (char)(0xe0|(c2>>12)));
                    //                 strbuf_push(buf, (char)(0x80|(c2>>6)));
                    //                 strbuf_push(buf, (char)(0x80|(c2&0x3f)));
                    //             }
                    //         }
                    //     } else {
                    //         strbuf_push(buf, (char)(0xf0|(c1>>2)));
                    //         strbuf_push(buf, (char)(0x80|((c1&0x3)<<4)|(c2>>12)));
                    //         strbuf_push(buf, (char)(0x80|((c2&0xfff)>>6)));
                    //         strbuf_push(buf, (char)(0x80|(c2&0x3f)));
                    //     }
                    // }
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
} ParserData;

typedef struct ParserToken {
    int type;
    long line, column;
    char* file;
    ParserData data;
} ParserToken;

static void printutf8(int bytes) {
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
    tok->type = type;
    tok->line = line;
    tok->column = column;
    tok->file = file;
    tok->data = data;
    return tok;
}
void parsertoken_print(ParserToken* tok) {
    printf("PToken {\n    type: %s,\n    loc: (%li, %li, %s),\n    ", CONTENTTYPEMAP[tok->type], tok->line, tok->column, tok->file);
    switch (tok->type) {
        case CONTENT_WORD:case CONTENT_SYM:
        case CONTENT_STR:printf("value: %s\n", tok->data.string);break;
        case CONTENT_CHAR:printf("value: ");printutf8(tok->data.character);printf("\n");break;
        case CONTENT_INT:printf("value: %li\n", tok->data.integer);break;
        case CONTENT_FLOT:printf("value: %f\n", tok->data.floating);break;
        case CONTENT_LINE:printf("no value\n");break;
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
    // if (new == '"' || new == '\'' || new == '\n' || new == '.' || new == ',' || new == ' ') return 0;
    return 0;
}

static Token* transform_parser_tree(char* treename, LinkedList* partree) {
    Token* semroot;
    {TokenData d = {.namespace={.name=strmove(treename),.childnode=NULL}};semroot=token_create(Nmsp, -1, -1, treename, d);}
    LinkedList* yardstack = LINKED_OBJECTS;
    todo();
    return semroot;
}

/*
mode can be 0 or 1
if mode is 1, inclusions will not be processed and a LinkedList of ParserTokens is returned
*/
Token* parse_file(char* fpath, char mode) {
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
    long line = 1, col = 1;
    // the line and column when the token was started
    long tline = 1, tcol = 1;
    long nlcol = 1;
    Token* fintree;
    #define location tline, tcol, fpath
    #define clocation line, col, fpath
    char last = 0;
    char c = 0;
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
        // if (c == '\n' && CONTENT_NOTSTR(content)) {
        //     if (lineend) {
        //         ParserData d = {.line='\n'};
        //         linkedlist_push(tokens, create_parsertoken(CONTENT_LINE, line, nlcol, fpath, d));
        //         lineend = 0;
        //         goto loop_continue;
        //     }
        //     lineend = 1;
        // } else {
        //     lineend = 0;
        // }
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
    {ParserData d = {.line='\n'};
    linkedlist_push(tokens, create_parsertoken(CONTENT_LINE, line, col-1, fpath, d));}
    // now transform the parser tokens into semantic tokens
    if (!mode) {
        fintree = transform_parser_tree(fpath, tokens);
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

void parser_test_ptokens(char* fpath) {
    LinkedList* list = (LinkedList*)parse_file(fpath, 1);
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

#endif