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

typedef struct LinkedListNode {
    struct LinkedListNode* next;
    struct LinkedListNode* prev;
    struct LinkedList* list;
    void* data;
} LinkedListNode;
typedef struct LinkedList {
    LinkedListNode* head;
    LinkedListNode* tail;
    size_t itemsize;
    uint size;
} LinkedList;

LinkedList* linkedlist_create(size_t itemsize) {
    LinkedList* list = (LinkedList*) malloc(sizeof(LinkedList));
    list->itemsize = itemsize;
    list->head = 0;
    list->tail = 0;
    list->size = 0;
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

#endif
