#ifndef __WATERCRESS_UTILS_H__
#define __WATERCRESS_UTILS_H__ 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "./types.h"

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
    char* dst = (char*)malloc(strlen(s1)+strlen(s2)+1);
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
void linkedlist_push(LinkedList* list, void* value) {
    LinkedListNode* node = linkedlist_init_node(list, value);
    if (list->head == 0) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size ++;
}
/*
pops the tail and returns the data pointer
returns zero if the list was empty
*/
void* linkedlist_pop(LinkedList* list) {
    if (list->tail == 0) {
        return 0;
    }
    list->size --;
    LinkedListNode* prev = list->tail->prev;
    void* data = list->tail->data;
    list->tail = prev;
    if (prev == 0) {
        list->head = 0;
    }
    free(prev);
    return data;
}
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
int linkedlist_insert_next(LinkedList* list, LinkedListNode* node, void* value) {
    if (node == 0) return -1;
    LinkedListNode* new = linkedlist_init_node(list, value);
    list->size ++;
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
/*
inserts a new node with the given value into the list directly before the given node
returns zero on success, -1 if the given node was null
*/
int linkedlist_insert_prev(LinkedList* list, LinkedListNode* node, void* value) {
    if (node == 0) return -1;
    LinkedListNode* new = linkedlist_init_node(list, value);
    list->size ++;
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
/*
inserts a new node with the given value at the specified index
returns zero on success, -1 if the index is out of bounds
*/
int linkedlist_insert_at(LinkedList* list, uint index, void* value) {
    if (index == list->size) {
        linkedlist_push(list, value);
        return 0;
    }
    if (index > list->size) {
        return -1;
    }
    LinkedListNode* cnode = linkedlist_get(list, index);
    return linkedlist_insert_prev(list, cnode, value);
}
/*
removes the node directly after the given node and returns its data pointer
returns zero if the given node was null or there was no node after it
*/
void* linkedlist_remove_next(LinkedList* list, LinkedListNode* node) {
    if (node == 0) return 0;
    LinkedListNode* next = node->next;
    if (next == 0) {
        return 0;
    }
    list->size --;
    if (next->next == 0) {
        list->tail = node;
        node->next = 0;
    } else {
        node->next = next->next;
        next->next->prev = node;
    }
    void* data = next->data;
    free(next);
    return data;
}
/*
removes the node directly before the given node and returns its data pointer
returns zero if the given node was null or there was no node before it
*/
void* linkedlist_remove_prev(LinkedList* list, LinkedListNode* node) {
    if (node == 0) return 0;
    LinkedListNode* prev = node->prev;
    if (prev == 0) {
        return 0;
    }
    list->size --;
    if (prev->prev == 0) {
        list->head = node;
        node->prev = 0;
    } else {
        node->prev = prev->prev;
        prev->prev->next = node;
    }
    void* data = prev->data;
    free(prev);
    return data;
}
/*
removes the node at the specified index and returns its data pointer
returns zero if the index was out of bounds
*/
void* linkedlist_remove_at(LinkedList* list, uint index) {
    if (index >= list->size) {
        return 0;
    }
    if (index == list->size-1) {
        return linkedlist_pop(list);
    }
    LinkedListNode* cnode = linkedlist_get(list, index+1);
    return linkedlist_remove_prev(list, cnode);
}

typedef struct DynList {
    uint len;
    uint cap;
    void** ptr;
    ItemEq eq;
} DynList;

DynList* dynlist_create(ItemEq eq) {
    DynList* list = (DynList*)malloc(sizeof(DynList));
    list->len = 0;
    list->cap = 2;
    list->ptr = (void**)malloc(sizeof(void*)*2);
    list->eq = eq;
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
        free(list->ptr[i]);
    }
    free(list->ptr);
    free(list);
}
void dynlist_expand(DynList* list) {
    list->cap *= 2;
    void** np = (void**)malloc(sizeof(void*)*(list->cap));
    for (uint i = 0; i < list->len; i ++) {
        np[i] = list->ptr[i];
    }
    // memcpy(np, list->ptr, (size_t)(list->len));
    free(list->ptr);
    list->ptr = np;
}
void dynlist_contract(DynList* list) {
    list->cap /= 2;
    void** np = (void**)malloc(sizeof(void*)*(list->cap));
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
int dynlist_push(DynList* list, void* value) {
    if (value == NULL) return -1;
    if (list->len == list->cap) {
        dynlist_expand(list);
    }
    (list->ptr)[list->len++] = value;
    return 0;
}
/*
removes and returns the value at the end of the list
returns NULL if the list was empty
*/
void* dynlist_pop(DynList* list) {
    if (list->len == 0) return NULL;
    void* ret = (list->ptr)[--list->len];
    if (list->len*4 <= list->cap) {
        dynlist_contract(list);
    }
    return ret;
}
/*
inserts the given value at the specified index
returns zero on success, -1 if the index was out of bounds or the given value was NULL
*/
int dynlist_insert(DynList* list, uint index, void* value) {
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
        for (uint i = index; i < (list->len - index); i ++) {
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
/*
removes and returns the value at the specified index
returns NULL if the index was out of bounds
*/
void* dynlist_remove(DynList* list, uint index) {
    if (index >= list->len) return NULL;
    list->len --;
    void* ret = list->ptr[index];
    if (list->len*4 <= list->cap) {
        list->cap /= 2;
        void** np = (void**)malloc(sizeof(void*)*(list->cap));
        if (index > 0) {
            for (uint i = 0; i < index; i ++) {
                np[i] = list->ptr[i];
            }
            // memcpy(np, list->ptr, index);
        }
        for (uint i = index; i < (list->len - index); i ++) {
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

typedef long(*HashFunc)(void*);

typedef struct HashMapEntry {
  void *key;
  void *value;
} HashMapEntry;

typedef struct HashMap {
  HashFunc h1;
  HashFunc h2;
  ItemEq eq;
  int size;
  int entries;
  HashMapEntry *table; 
} HashMap;

HashMap *hashmap_create(HashFunc h1, HashFunc h2, ItemEq eq) {
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    map->h1 = h1;
    map->h2 = h2;
    map->eq = eq;
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
            free(map->table[i].value);
            free(map->table[i].key);
        }
    }
    free(map->table);
    free(map);
}

void hashmap_expand(HashMap *map);

void hashmap_set(HashMap *map, void *key, void *value) {
    int a = (map->h1)(key);
    int b = (map->h2)(key);

    for (int i = 0; i < map->entries+1; i++) {
        int index = (a+b*i) % (map->size);
        if (map->table[index].key == NULL || (map->eq)(map->table[index].key, key)) {
            map->table[index].key = key;
            map->table[index].value = value;
            map->entries++;

            if (map->entries * 2 > map->size) {
                hashmap_expand(map);
            }
            return;
        }
    }

    // there has been a BIG error. don't do anything...
    return;
}

void *hashmap_get(HashMap *map, void *key) {
    int a = (map->h1)(key);
    int b = (map->h2)(key);
    for (int i = 0; i < map->entries; i++) {
        int index = (a+b*i) % (map->size);
        if ((map->eq)(map->table[index].key, key)) {
            return map->table[index].value;
        } else if (map->table[index].key == NULL) {
            return NULL;
        }
    }

    return NULL;
}

void hashmap_expand(HashMap *map) {
    HashMapEntry *old_table = map->table;
    int old_size = map->size;
    map->size = map->size * 2 + 1;
    map->table = (HashMapEntry *)malloc(sizeof(HashMapEntry)*(map->size));

    for (int i = 0; i < map->size; i++) {
        map->table[i].key = NULL;
    }

    
    for (int i = 0; i < old_size; i++) {
        if (old_table[i].key) {
            hashmap_set(map, old_table[i].key, old_table[i].value);
        }
    }

    free(old_table);
}

HashMap *hashmap_copy(HashMap *original) {
    HashMap *new = hashmap_create(original->h1, original->h2, original->eq);

    for (int i = 0; i < original->size; i++) {
        if (original->table[i].key != NULL) {
            hashmap_set(new, original->table[i].key, original->table[i].value);
        }
    }

    return new;
}

int streq(void *a, void *b) {
    return !strcmp(a, b);
}

long hashstr(void* strp) {
    ubyte* str = (ubyte*)strp;
    ulong hash = 0;
    int c;
    while ((c = *str++) != 0) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return *((long*)&hash);
}

long hashstr2(void* strp) {
    ubyte* str = (ubyte*)strp;
    ulong hash = 0, h;
    ubyte c;
    while ((c = *str++) != 0) {
        hash = (hash << 4) + c;
        if ((h = hash & 0xf0000000)) {
            hash ^= h >> 24;
        }
        hash &= ~h;
    }
    return *((long*)&h);
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

long ident(void *a) {
    return (long)a;
}

#endif
