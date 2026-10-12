// wpaudio/list.h - Emulation stub for WPAudio list functions

#ifndef _WPAUDIO_LIST_H_
#define _WPAUDIO_LIST_H_

// Basic linked list emulation
typedef struct ListNode {
    void *data;
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

typedef struct {
    ListNode *head;
    ListNode *tail;
    int count;
} List;

static inline void List_Init(List *list) {
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

static inline void List_Add(List *list, void *data) {
    ListNode *node = (ListNode*)malloc(sizeof(ListNode));
    if (node) {
        node->data = data;
        node->next = NULL;
        node->prev = list->tail;
        if (list->tail) list->tail->next = node;
        list->tail = node;
        if (!list->head) list->head = node;
        list->count++;
    }
}

static inline void List_Remove(List *list, ListNode *node) {
    if (node->prev) node->prev->next = node->next;
    else list->head = node->next;
    if (node->next) node->next->prev = node->prev;
    else list->tail = node->prev;
    free(node);
    list->count--;
}

static inline void List_Clear(List *list) {
    ListNode *node = list->head;
    while (node) {
        ListNode *next = node->next;
        free(node);
        node = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

#endif // _WPAUDIO_LIST_H_
