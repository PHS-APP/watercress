#ifndef __WATERCRESS_UTILS_H__
#define __WATERCRESS_UTILS_H__ 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "./types.h"

// whether to print markers
char _DBG_MARKER = 0;
#define debugf(enable, ...) if(enable)printf(__VA_ARGS__)
#define marker(...) debugf(_DBG_MARKER, "\nMARKER[%d (%s){%s}]: ", __LINE__, __func__, __FILE__);debugf(_DBG_MARKER, __VA_ARGS__);debugf(_DBG_MARKER, "\n")

// data structure tracing bit fields
static char _DBG_DATASTRUCTURE_BASE = 0;
static char _DBG_DATASTRUCTURE_MASK = 127;

#define DATASTRUCTDBG_LINKEDLIST 1
#define DATASTRUCTDBG_DYNLIST 2
#define DATASTRUCTDBG_HASHMAP 4


#define SRCLOC long line, const char* func, const char* file

// DO NOT USE THIS, should only be called based on CLI args, this has already been handled
void datastruct_debug_enable_base(char which) {
    _DBG_DATASTRUCTURE_BASE |= which;
}
// DO NOT USE THIS, should only be called based on CLI args, this has already been handled
void datastruct_debug_disable_base(char which) {
    _DBG_DATASTRUCTURE_BASE &= ~which;
}
// allows trace printing that is known to be irrelevant in ALL CASES to be hidden
// WARNING, only use for code which is KNOWN AND TESTED to function properly in ALL cases
void datastruct_debug_enable_mask(char which) {
    _DBG_DATASTRUCTURE_MASK |= which;
}
// WARNING, only use for code which is KNOWN AND TESTED to function properly in ALL cases
void datastruct_debug_disable_mask(char which) {
    _DBG_DATASTRUCTURE_MASK &= ~which;
}

#define _DBG_DATASTRUCTURE (_DBG_DATASTRUCTURE_BASE&_DBG_DATASTRUCTURE_MASK)
#define LINKEDLIST_DEBUG (_DBG_DATASTRUCTURE&1)
#define DYNLIST_DEBUG (_DBG_DATASTRUCTURE&2)
#define HASHMAP_DEBUG (_DBG_DATASTRUCTURE&4)

void _todo(ulong line, const char* file, const char* func) {
    printf("TODO ENCOUNTERED (line %lu of file '%s' function '%s')\n", line, file, func);
    exit(1);
}
#define todo() _todo(__LINE__, __FILE__, __func__)

char* strmove(const char* str) {
    char* dst = (char*)malloc(strlen(str)+1);
    if (dst == NULL) {
        return NULL;
    }
    strcpy(dst, str);
    return dst;
}

char* strjoin(const char* s1, const char* s2) {
    char* dst = (char*)calloc(strlen(s1)+strlen(s2)+1, sizeof(char));
    if (dst == NULL) {
        return NULL;
    }
    strcpy(dst, s1);
    strcpy(dst+strlen(s1), s2);
    return dst;
}

int startswith(const char* str, const char* pre) {
    int c = 0;
    char c1;
    char c2;
    while ((c1 = str[c]) != 0 && (c2 = pre[c]) != 0) {
        if (c1 != c2) {
            return 0;
        }
        c ++;
    }
    if (c2 == 0) {
        return 1;
    }
    return 0;
}

typedef int(*ItemEq)(void*, void*);

int pointereq(void* p1, void* p2) {
    return p1 == p2;
}

typedef struct LinkedListNode {
    struct LinkedListNode* next;
    struct LinkedListNode* prev;
    struct LinkedList* list;
    void* data;
} LinkedListNode;
typedef struct LinkedList {
    LinkedListNode* head;
    LinkedListNode* tail;
    uint size;
    ItemEq eq;
} LinkedList;

LinkedList* linkedlist_create(ItemEq eq) {
    LinkedList* list = (LinkedList*) malloc(sizeof(LinkedList));
    list->head = 0;
    list->tail = 0;
    list->size = 0;
    list->eq = eq;
    return list;
}
void linkedlist_destroy(LinkedList* list) {
    LinkedListNode* curr = list->head;
    LinkedListNode* next = 0;
    while (curr) {
        next = curr->next;
        // printf("C=(%p) N=(%p) D=(%p)\n", (void*)curr, (void*)next, curr->data);
        if (curr->data) {
            free(curr->data);
        }
        free(curr);
        curr = next;
    }
    free(list);
}
LinkedListNode* linkedlist_init_node(LinkedList* list, void* value) {
    LinkedListNode* node = (LinkedListNode*) malloc(sizeof(LinkedListNode));
    node->list = list;
    node->data = value;
    node->next = 0;
    node->prev = 0;
    return node;
}
void linkedlist_destroy_node(LinkedListNode* node) {
    if (node->data != 0) {
        free(node->data);
    }
    free(node);
}
/*
returns the 1-based index of the value in the list
returns zero if the value was not found
*/
uint linkedlist_indexof(LinkedList* list, void* value) {
    LinkedListNode* curr = list->head;
    uint i = 1;
    while (curr) {
        if (list->eq(curr->data, value)) {
            return i;
        }
        i ++;
        curr = curr->next;
    }
    return 0;
}
/*
creates a new node with the given data pointer and appends it to the list
*/
void _linkedlist_push(LinkedList* list, void* value, long line, const char* func, const char* file) {
    debugf(LINKEDLIST_DEBUG, "LLPU[%ld (%s){%s}]: [%d] D=(%p) ", line, func, file, list->size, value);
    // if (!LINKEDLIST_DEBUG)printf("LLPU[%ld (%s){%s}]: [%d] (%p) ", line, func, file, list->size, value);
    LinkedListNode* node = linkedlist_init_node(list, value);
    if (list->head == 0) {
        debugf(LINKEDLIST_DEBUG, "NPL");
        // if (!LINKEDLIST_DEBUG)printf("NPL");
        list->head = node;
        list->tail = node;
    } else {
        debugf(LINKEDLIST_DEBUG, "YPL=(%p)", (void*)list->tail);
        // if (!LINKEDLIST_DEBUG)printf("YPL=(%p)", (void*)list->tail);
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    debugf(LINKEDLIST_DEBUG, " CH=(%p) CT=(%p)\n", (void*)list->head, (void*)list->tail);
    // if (!LINKEDLIST_DEBUG)printf(" CH=(%p) CT=(%p)\n", (void*)list->head, (void*)list->tail);
    list->size ++;
}
#define linkedlist_push(list, value) _linkedlist_push(list, value, __LINE__, __func__, __FILE__);
/*
pops the tail and returns the data pointer
returns zero if the list was empty
*/
void* _linkedlist_pop(LinkedList* list, long line, const char* func, const char* file) {
    debugf(LINKEDLIST_DEBUG, "LLPO[%ld (%s){%s}]: [%d] ", line, func, file, list->size);
    // if (!LINKEDLIST_DEBUG)printf("LLPO[%ld (%s){%s}]: [%d] ", line, func, file, list->size);
    if (list->tail == 0) {
        return 0;
    }
    list->size --;
    LinkedListNode* prev = list->tail->prev;
    debugf(LINKEDLIST_DEBUG, "T=(%p) TP=(%p) ", (void*)list->tail, (void*)prev);
    // if (!LINKEDLIST_DEBUG)printf("T=(%p) TP=(%p) ", (void*)list->tail, (void*)prev);
    void* data = list->tail->data;
    debugf(LINKEDLIST_DEBUG, "D=(%p)\n", data);
    free(list->tail);
    // if (!LINKEDLIST_DEBUG)printf("D=(%p)\n", data);
    list->tail = prev;
    if (prev == 0) {
        list->head = 0;
    } else {
        prev->next = 0;
    }
    // free(prev);
    return data;
}
#define linkedlist_pop(list) _linkedlist_pop(list, __LINE__, __func__, __FILE__)
/*
returns the node at the specified index
returns zero if the index was out of bounds
*/
LinkedListNode* linkedlist_get(LinkedList* list, uint index) {
    if (index >= list->size) {
        return 0;
    }
    if (index > (list->size/2)) {
        LinkedListNode* curr = list->tail;
        uint cnt = list->size - index - 1;
        for (uint i = 0; i < cnt; i ++) {
            curr = curr->prev;
        }
        return curr;
    } else {
        LinkedListNode* curr = list->head;
        for (uint i = 0; i < index; i ++) {
            curr = curr->next;
        }
        return curr;
    }
}
/*
sets the node at index to the given node and returns the old one
returns zero if either the new node was null or the index was out of bounds
*/
LinkedListNode* linkedlist_set(LinkedList* list, uint index, LinkedListNode* node) {
    if (node == 0) {
        return 0;
    }
    LinkedListNode* oldnode = linkedlist_get(list, index);
    if (oldnode == 0) {
        return 0;
    }
    LinkedListNode* prev = oldnode->prev;
    LinkedListNode* next = oldnode->next;
    if (prev != 0) {
        prev->next = node;
    }
    if (next != 0) {
        next->prev = node;
    }
    node->next = next;
    node->prev = prev;
    return oldnode;
}
/*
inserts a new node with the given value into the list directly after the given node
returns zero on success, -1 if the given node was null
*/
int _linkedlist_insert_next(LinkedList* list, LinkedListNode* node, void* value, SRCLOC) {
    debugf(LINKEDLIST_DEBUG, "LLIN[%ld (%s){%s}]: [%d] N=(%p) D=(%p)", line, func, file, list->size, (void*)node, value);
    if (node == 0) {debugf(LINKEDLIST_DEBUG, "\n");return -1;}
    LinkedListNode* new = linkedlist_init_node(list, value);
    list->size ++;
    debugf(LINKEDLIST_DEBUG, " NN=(%p)\n", (void*)node->next);
    if (node->next) {
        node->next->prev = new;
        new->next = node->next;
    } else {
        list->tail = new;
    }
    node->next = new;
    new->prev = node;
    return 0;
}
#define linkedlist_insert_next(list, node, value) _linkedlist_insert_next(list, node, value, __LINE__, __func__, __FILE__)
/*
inserts a new node with the given value into the list directly before the given node
returns zero on success, -1 if the given node was null
*/
int _linkedlist_insert_prev(LinkedList* list, LinkedListNode* node, void* value, SRCLOC) {
    debugf(LINKEDLIST_DEBUG, "LLIP[%ld (%s){%s}]: [%d] N=(%p) D=(%p)", line, func, file, list->size, (void*)node, value);
    if (node == 0) {debugf(LINKEDLIST_DEBUG, "\n");return -1;}
    LinkedListNode* new = linkedlist_init_node(list, value);
    list->size ++;
    debugf(LINKEDLIST_DEBUG, " NP=(%p)\n", (void*)node->prev);
    if (node->prev) {
        node->prev->next = new;
        new->prev = node->prev;
    } else {
        list->head = new;
    }
    node->prev = new;
    new->next = node;
    return 0;
}
#define linkedlist_insert_prev(list, node, value) _linkedlist_insert_prev(list, node, value, __LINE__, __func__, __FILE__)
/*
inserts a new node with the given value at the specified index
returns zero on success, -1 if the index is out of bounds
*/
int _linkedlist_insert_at(LinkedList* list, uint index, void* value, SRCLOC) {
    debugf(LINKEDLIST_DEBUG, "LLIA[%ld (%s){%s}]: [%d] I=(%d) D=(%p)", line, func, file, list->size, index, value);
    if (index == list->size) {
        debugf(LINKEDLIST_DEBUG, "\n");
        linkedlist_push(list, value);
        return 0;
    }
    if (index > list->size) {
        return -1;
    }
    LinkedListNode* cnode = linkedlist_get(list, index);
    debugf(LINKEDLIST_DEBUG, " GN=(%p)\n", (void*)cnode);
    return linkedlist_insert_prev(list, cnode, value);
}
#define linkedlist_insert_at(list, index, value) _linkedlist_insert_at(list, index, value, __LINE__, __func__, __FILE__)
/*
removes the node directly after the given node and returns its data pointer
returns zero if the given node was null or there was no node after it
*/
void* _linkedlist_remove_next(LinkedList* list, LinkedListNode* node, SRCLOC) {
    debugf(LINKEDLIST_DEBUG, "LLRN[%ld (%s){%s}]: [%d] N=(%p)", line, func, file, list->size, (void*)node);
    if (node == 0) {debugf(LINKEDLIST_DEBUG, "\n");return 0;}
    LinkedListNode* next = node->next;
    debugf(LINKEDLIST_DEBUG, " NN=(%p)", (void*)node->next);
    if (next == 0) {
        debugf(LINKEDLIST_DEBUG, "\n");
        return 0;
    }
    list->size --;
    if (next->next == 0) {
        list->tail = node;
        node->next = 0;
    } else {
        debugf(LINKEDLIST_DEBUG, " NNN=(%p)", (void*)next->next);
        node->next = next->next;
        next->next->prev = node;
    }
    debugf(LINKEDLIST_DEBUG, "\n");
    void* data = next->data;
    free(next);
    return data;
}
#define linkedlist_remove_next(list, node) _linkedlist_remove_next(list, node, __LINE__, __func__, __FILE__)
/*
removes the node directly before the given node and returns its data pointer
returns zero if the given node was null or there was no node before it
*/
void* _linkedlist_remove_prev(LinkedList* list, LinkedListNode* node, SRCLOC) {
    debugf(LINKEDLIST_DEBUG, "LLRP[%ld (%s){%s}]: [%d] N=(%p)", line, func, file, list->size, (void*)node);
    if (node == 0) {debugf(LINKEDLIST_DEBUG, "\n");return 0;}
    LinkedListNode* prev = node->prev;
    debugf(LINKEDLIST_DEBUG, " NP=(%p)", (void*)node->prev);
    if (prev == 0) {
        debugf(LINKEDLIST_DEBUG, "\n");
        return 0;
    }
    list->size --;
    if (prev->prev == 0) {
        list->head = node;
        node->prev = 0;
    } else {
        debugf(LINKEDLIST_DEBUG, " NPP=(%p)", (void*)prev->prev);
        node->prev = prev->prev;
        prev->prev->next = node;
    }
    debugf(LINKEDLIST_DEBUG, "\n");
    void* data = prev->data;
    free(prev);
    return data;
}
#define linkedlist_remove_prev(list, node) _linkedlist_remove_prev(list, node, __LINE__, __func__, __FILE__)
/*
removes the node at the specified index and returns its data pointer
returns zero if the index was out of bounds
*/
void* _linkedlist_remove_at(LinkedList* list, uint index, long line, const char* func, const char* file) {
    debugf(LINKEDLIST_DEBUG, "LLRA[%ld (%s){%s}]: [%d] I=(%d)", line, func, file, list->size, index);
    // if (!LINKEDLIST_DEBUG)printf("LLRA[%ld (%s){%s}]: [%d] I=(%d)", line, func, file, list->size, index);
    if (index >= list->size) {
        return 0;
    }
    if (index == list->size-1) {
        debugf(LINKEDLIST_DEBUG, "\n");
        // if (!LINKEDLIST_DEBUG)printf("\n");
        return linkedlist_pop(list);
    }
    LinkedListNode* cnode = linkedlist_get(list, index+1);
    debugf(LINKEDLIST_DEBUG, " GN=(%p)\n", (void*)cnode);
    // if (!LINKEDLIST_DEBUG)printf(" GN=(%p)\n", (void*)cnode);
    return linkedlist_remove_prev(list, cnode);
}
#define linkedlist_remove_at(list, index) _linkedlist_remove_at(list, index, __LINE__, __func__, __FILE__)

typedef void(*ItemRel)(void*);

typedef struct DynList {
    uint len;
    uint cap;
    void** ptr;
    ItemEq eq;
    ItemRel rel;
} DynList;

void no_release(void* _) {}

DynList* dynlist_create(ItemEq eq, ItemRel rel) {
    DynList* list = (DynList*)malloc(sizeof(DynList));
    list->len = 0;
    list->cap = 2;
    list->ptr = (void**)malloc(sizeof(void*)*2);
    list->eq = eq;
    list->rel = rel;
    return list;
}
/*
returns the one-based index of the given value in the list
returns zero if the value was not found
*/
uint dynlist_indexof(DynList* list, void* value) {
    for (uint i = 0; i < list->len; i ++) {
        if (list->eq(list->ptr[i], value)) {
            return i+1;
        }
    }
    return 0;
}
void dynlist_destroy(DynList* list) {
    for (uint i = 0; i < list->len; i ++) {
        list->rel(list->ptr[i]);
    }
    free(list->ptr);
    free(list);
}
void dynlist_clear(DynList* list) {
    free(list->ptr);
    list->ptr = (void**)malloc(sizeof(void*)*2);
    list->len = 0;
    list->cap = 2;
}
void dynlist_detach(DynList* list) {
    list->ptr = (void**)malloc(sizeof(void*)*2);
    list->len = 0;
    list->cap = 2;
}
DynList* dynlist_reown(DynList* list) {
    DynList* nl = dynlist_create(list->eq, list->rel);
    nl->len = list->len;
    nl->cap = list->cap;
    nl->ptr = list->ptr;
    dynlist_detach(list);
    return nl;
}
void dynlist_expand(DynList* list) {
    list->cap *= 2;
    void** np = (void**)calloc(list->cap, sizeof(void*));
    for (uint i = 0; i < list->len; i ++) {
        np[i] = list->ptr[i];
    }
    // memcpy(np, list->ptr, (size_t)(list->len));
    free(list->ptr);
    list->ptr = np;
}
void dynlist_contract(DynList* list) {
    if (list->cap == 2) return;
    list->cap /= 2;
    void** np = (void**)calloc(list->cap, sizeof(void*));
    for (uint i = 0; i < list->len; i ++) {
        np[i] = list->ptr[i];
    }
    // memcpy(np, list->ptr, (size_t)(list->len));
    free(list->ptr);
    list->ptr = np;
}
/*
returns the value at the specified index
returns NULL if the index was out of bounds
*/
void* dynlist_get(DynList* list, uint index) {
    if (index >= list->len) return NULL;
    return list->ptr[index];
}
/*
sets the specified index to the given value
returns zero on success, -1 if the index was out of bounds or the value was NULL
*/
int dynlist_set(DynList* list, uint index, void* value) {
    if (index >= list->len || value == NULL) return -1;
    list->ptr[index] = value;
    return 0;
}
/*
appends the given value to the end of the list
returns zero on success, -1 if the value was NULL
*/
int _dynlist_push(DynList* list, void* value, SRCLOC) {
    debugf(DYNLIST_DEBUG, "DLPU[%ld (%s){%s}]: [%d/%d] D=(%p)\n", line, func, file, list->len, list->cap, value);
    if (value == NULL) return -1;
    if (list->len == list->cap) {
        dynlist_expand(list);
    }
    (list->ptr)[list->len++] = value;
    return 0;
}
#define dynlist_push(list, value) _dynlist_push(list, value, __LINE__, __func__, __FILE__)
/*
removes and returns the value at the end of the list
returns NULL if the list was empty
*/
void* _dynlist_pop(DynList* list, SRCLOC) {
    debugf(DYNLIST_DEBUG, "DLPO[%ld (%s){%s}]: [%d/%d]", line, func, file, list->len, list->cap);
    if (list->len == 0) {debugf(DYNLIST_DEBUG, "\n");return NULL;}
    void* ret = (list->ptr)[--list->len];
    debugf(DYNLIST_DEBUG, " D=(%p)\n", ret);
    if (list->len*4 <= list->cap) {
        dynlist_contract(list);
    } else {
        (list->ptr)[list->len] = 0;
    }
    return ret;
}
#define dynlist_pop(list) _dynlist_pop(list, __LINE__, __func__, __FILE__)
/*
inserts the given value at the specified index
returns zero on success, -1 if the index was out of bounds or the given value was NULL
*/
int _dynlist_insert(DynList* list, uint index, void* value, SRCLOC) {
    debugf(DYNLIST_DEBUG, "DLIN[%ld (%s){%s}]: [%d/%d] I=(%d) D=(%p)\n", line, func, file, list->len, list->cap, index, value);
    if (index > list->len || value == NULL) return -1;
    if (index == list->len) {
        dynlist_push(list, value);
        return 0;
    }
    if (list->len == list->cap) {
        list->cap *= 2;
        void** np = (void**)malloc(sizeof(void*)*(list->cap));
        if (index > 0) {
            for (uint i = 0; i < index; i ++) {
                np[i] = list->ptr[i];
            }
            // memcpy(np, list->ptr, (size_t)index);
        }
        for (uint i = index; i < list->len; i ++) {
            np[i+1] = list->ptr[i];
        }
        // memcpy(np+index+1, list->ptr+index, (size_t)(list->len-index));
        np[index] = value;
        free(list->ptr);
        list->ptr = np;
        list->len ++;
        return 0;
    }
    for (uint i = (list->len-1); i >= index; i --) {
        list->ptr[i+1] = list->ptr[i];
    }
    list->ptr[index] = value;
    list->len ++;
    return 0;
}
#define dynlist_insert(list, index, value) _dynlist_insert(list, index, value, __LINE__, __func__, __FILE__)
/*
removes and returns the value at the specified index
returns NULL if the index was out of bounds
*/
void* _dynlist_remove(DynList* list, uint index, SRCLOC) {
    debugf(DYNLIST_DEBUG, "DLRE[%ld (%s){%s}]: [%d/%d] I=(%d)", line, func, file, list->len, list->cap, index);
    if (index >= list->len) {debugf(DYNLIST_DEBUG, "\n");return NULL;}
    if (index == list->len - 1) {debugf(DYNLIST_DEBUG, "\n");return dynlist_pop(list);}
    list->len --;
    void* ret = list->ptr[index];
    debugf(DYNLIST_DEBUG, " D=(%p)\n", ret);
    if (list->len*4 <= list->cap) {
        list->cap /= 2;
        void** np = (void**)malloc(sizeof(void*)*(list->cap));
        if (index > 0) {
            for (uint i = 0; i < index; i ++) {
                np[i] = list->ptr[i];
            }
            // memcpy(np, list->ptr, index);
        }
        for (uint i = index; i < list->len; i ++) {
            np[i] = list->ptr[i+1];
        }
        // memcpy(np+index, list->ptr+index+1, (size_t)(list->len-index));
        free(list->ptr);
        list->ptr = np;
        return ret;
    }
    for (uint i = index; i < list->len; i ++) {
        list->ptr[i] = list->ptr[i+1];
    }
    return ret;
}
#define dynlist_remove(list, index) _dynlist_remove(list, index, __LINE__, __func__, __FILE__)
void dynlist_inspect(DynList* list) {
    printf("(%p)[%d/%d]\n", (void*)list->ptr, list->len, list->cap);
    for (int i = 0; i < list->cap; i ++) {
        printf("(%p) ", list->ptr[i]);
    }
    printf("\n");
}

/*
returns a DynList with the same contents as the LinkedList
the LinkedList is destroyed
*/
DynList* linkedlist_flatten(LinkedList* list) {
    uint l = 2;
    while (l < list->size) {
        l = l << 1;
    }
    DynList* dlist = (DynList*)malloc(sizeof(dlist));
    dlist->eq = list->eq;
    dlist->cap = l;
    dlist->len = list->size;
    dlist->ptr = (void**)malloc(sizeof(void*)*(list->size));
    uint i = 0;
    LinkedListNode* curr = list->head;
    LinkedListNode* next = 0;
    while (curr) {
        dlist->ptr[i++] = curr->data;
        next = curr->next;
        free(curr);
        curr = next;
    }
    free(list);
    return dlist;
}

typedef struct StrBuf {
    size_t cap;
    size_t len;
    char* ptr;
} StrBuf;

StrBuf* strbuf_create(void) {
    StrBuf* buf = (StrBuf*)malloc(sizeof(StrBuf));
    buf->cap = 8;
    buf->len = 0;
    buf->ptr = (char*)calloc(8, sizeof(char));
    return buf;
}
void strbuf_destroy(StrBuf* buf) {
    free(buf->ptr);
    free(buf);
}
void strbuf_push(StrBuf* buf, char c) {
    if (buf->len == (buf->cap-1)) {
        buf->cap *= 2;
        char* np = (char*)malloc(sizeof(char)*(buf->cap));
        for (size_t i = 0; i < buf->len; i ++) {
            np[i] = buf->ptr[i];
        }
        free(buf->ptr);
        buf->ptr = np;
    }
    buf->ptr[buf->len++] = c;
}
/*
resets the strbuf
*/
void strbuf_discard(StrBuf* buf) {
    if (buf->cap == 8) {
        for (size_t i = 0; i < buf->len; i ++) {
            buf->ptr[i] = 0;
        }
    } else {
        buf->cap = 8;
        free(buf->ptr);
        buf->ptr = (char*)calloc(8, sizeof(char));
    }
    buf->len = 0;
}
/*
creates and returns a new char buf that contains all the characters with no extra space
resets the buffer to default
*/
char* strbuf_consume(StrBuf* buf) {
    char* cbuf = (char*)calloc(buf->len+1, sizeof(char));
    for (size_t i = 0; i < buf->len; i ++) {
        cbuf[i] = buf->ptr[i];
    }
    free(buf->ptr);
    buf->ptr = (char*)calloc(8, sizeof(char));
    buf->cap = 8;
    buf->len = 0;
    return cbuf;
}

typedef ulong(*HashFunc)(void*);

typedef struct HashMapEntry {
  void *key;
  void *value;
} HashMapEntry;

typedef struct HashMap {
  HashFunc h1;
  HashFunc h2;
  ItemEq eq;
  ItemRel rel;
  int size;
  int entries;
  HashMapEntry *table; 
} HashMap;

HashMap *hashmap_create(HashFunc h1, HashFunc h2, ItemEq eq) {
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    map->h1 = h1;
    map->h2 = h2;
    map->eq = eq;
    map->rel = free;
    map->size = 17;
    map->entries = 0;
    map->table = (HashMapEntry *)malloc(sizeof(HashMapEntry)*map->size);

    for (int i = 0; i < map->size; i++) {
        map->table[i].key = NULL;
    }

    return map;
}

void hashmap_destroy(HashMap *map) {
    for (int i = 0; i < map->size; i ++) {
        if (map->table[i].key != NULL) {
            map->rel(map->table[i].value);
            map->rel(map->table[i].key);
        }
    }
    free(map->table);
    free(map);
}

void hashmap_expand(HashMap *map);

void _hashmap_set(HashMap *map, void *key, void *value, SRCLOC) {
    debugf(HASHMAP_DEBUG, "HMSE[%ld (%s){%s}]: [%d/%d] K=(%p) V=(%p)", line, func, file, map->entries, map->size, key, value);
    if (key == NULL) {
        debugf(HASHMAP_DEBUG, "\n");
        puts("oops");
        return;
    }

    ulong a = (map->h1)(key);
    ulong b = (map->h2)(key);
    debugf(HASHMAP_DEBUG, " a=%#lx b=%#lx", a, b);

    for (int i = 0; i < map->entries+1; i++) {
        ulong index = (a+b*i) % (map->size);
        if (map->table[index].key == NULL || (map->eq)(map->table[index].key, key)) {
            debugf(HASHMAP_DEBUG, " I=(%d) S=(%lu)", i, index);
            map->table[index].key = key;
            map->table[index].value = value;
            map->entries++;

            if (map->entries * 2 > map->size) {
                debugf(HASHMAP_DEBUG, " EX");
                hashmap_expand(map);
            } else {
                debugf(HASHMAP_DEBUG, " NEX");
            }
            debugf(HASHMAP_DEBUG, "\n");
            return;
        }
    }
    debugf(HASHMAP_DEBUG, " S=(NO SPACE)\n");

    // there has been a BIG error. don't do anything...
    return;
}
#define hashmap_set(map, key, value) _hashmap_set(map, key, value, __LINE__, __func__, __FILE__)

void *_hashmap_get(HashMap *map, void *key, SRCLOC) {
    debugf(HASHMAP_DEBUG, "HMGE[%ld (%s){%s}]: [%d/%d] K=(%p)", line, func, file, map->entries, map->size, key);
    ulong a = (map->h1)(key);
    ulong b = (map->h2)(key);
    debugf(HASHMAP_DEBUG, " a=%#lx b=%#lx", a, b);
    for (int i = 0; i < map->entries; i++) {
        ulong index = (a+b*i) % (map->size);
        if (map->table[index].key == NULL) {
            debugf(HASHMAP_DEBUG, " I=(%d) S=(%lu) V=(%p)\n", i, index, NULL);
            return NULL;
        } else if ((map->eq)(map->table[index].key, key)) {
            debugf(HASHMAP_DEBUG, " I=(%d) S=(%lu) V=(%p)\n", i, index, map->table[index].value);
            return map->table[index].value;
        } 
    }
    debugf(HASHMAP_DEBUG, " S=(NOT FOUND)\n");

    return NULL;
}
#define hashmap_get(map, key) _hashmap_get(map, key, __LINE__, __func__, __FILE__)

void hashmap_expand(HashMap *map) {
    datastruct_debug_disable_mask(DATASTRUCTDBG_HASHMAP);
    HashMapEntry *old_table = map->table;
    int old_size = map->size;
    map->size = map->size * 2 + 1;
    map->table = (HashMapEntry *)malloc(sizeof(HashMapEntry)*(map->size));

    for (int i = 0; i < map->size; i++) {
        map->table[i].key = NULL;
    }

    
    for (int i = 0; i < old_size; i++) {
        if (old_table[i].key != NULL) {
            hashmap_set(map, old_table[i].key, old_table[i].value);
        }
    }
    datastruct_debug_enable_mask(DATASTRUCTDBG_HASHMAP);

    free(old_table);
}

HashMap *hashmap_copy(HashMap *original) {
    datastruct_debug_disable_mask(DATASTRUCTDBG_HASHMAP);
    HashMap *new = hashmap_create(original->h1, original->h2, original->eq);
    new->rel = original->rel;

    for (int i = 0; i < original->size; i++) {
        if (original->table[i].key != NULL) {
            hashmap_set(new, original->table[i].key, original->table[i].value);
        }
    }
    datastruct_debug_enable_mask(DATASTRUCTDBG_HASHMAP);

    return new;
}

int streq(void *a, void *b) {
    return !strcmp(a, b);
}

ulong hashstr(void* strp) {
    ubyte* str = (ubyte*)strp;
    ulong hash = 0;
    int c;
    while ((c = *str++) != 0) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

ulong hashstr2(void* strp) {
    ubyte* str = (ubyte*)strp;
    ulong hash = 0, h = 0;
    ubyte c;
    while ((c = *str++) != 0) {
        hash = (hash << 4) + c;
        if ((h = hash & 0xf0000000)) {
            hash ^= h >> 24;
        }
        hash &= ~h;
    }
    return hash;
}

long hashsized(void* vdata, size_t size) {
    ubyte* data = (ubyte*) vdata;
    // printf("data size: %li\n", size);
    ulong hash = 0;
    int c;
    while (size) {
        c = *(data++);
        // printf("%02x ", c);
        hash = c + (hash << 6) + (hash << 16) - hash;
        size --;
    }
    // putchar('\n');
    return *((long*)&hash);
}

int inteq(void *a, void *b) {
    return *((int*)a) == *((int*)b);
}

ulong ident(void *a) {
    return (ulong)a;
}

#undef _DBG_DATASTRUCTURE

#endif
