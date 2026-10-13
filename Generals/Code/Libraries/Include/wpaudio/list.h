// wpaudio/list.h - Circular doubly-linked list
// Original Westwood Studios implementation

#ifndef _WPAUDIO_LIST_H_
#define _WPAUDIO_LIST_H_

#include <wpaudio/altypes.h>

// Priority type - must be large enough to hold a pointer value
// (used for head node identification: head->pri = (Priority)head)
typedef uintptr_t Priority;

// List node - circular doubly-linked list
// prev/next point to other nodes; a node points to itself when not in a list
typedef struct ListNode {
    struct ListNode *prev;
    struct ListNode *next;
    Priority pri;
} ListNode;

// List head - same memory layout as ListNode so it can be cast
// pri stores (Priority)head to identify it as a head node
typedef struct ListHead {
    struct ListNode *prev;
    struct ListNode *next;
    Priority pri;
} ListHead;

// Inline helpers
static inline int ListNodeIsHead(ListNode *node)
{
    return node->pri == (Priority)(uintptr_t)node;
}

static inline int ListNodeInList(ListNode *node)
{
    return node && (node->next != node);
}

// Add node to tail of list
static inline void ListAddToTail(ListHead *head, ListNode *node)
{
    node->prev = head->prev;
    node->next = (ListNode *)head;
    head->prev->next = node;
    head->prev = node;
}

// Implemented in AUD_List.cpp
void ListInit(ListHead *head);
void ListNodeInit(ListNode *node);
void ListNodeInsert(ListNode *node, ListNode *new_node);
void ListNodeAppend(ListNode *node, ListNode *new_node);
void ListNodeRemove(ListNode *node);
ListNode *ListNodeNext(ListNode *node);
ListNode *ListNodePrev(ListNode *node);
ListNode *ListNodeLoopNext(ListNode *node);
ListNode *ListNodeLoopPrev(ListNode *node);
int ListAddNodeSortAscending(ListHead *head, ListNode *new_node);
void ListAddNode(ListHead *head, ListNode *new_node);
void ListAddNodeAfter(ListHead *head, ListNode *new_node);
void ListMerge(ListHead *from, ListHead *to);
int ListCountItems(ListHead *head);
ListNode *ListFirstItem(ListHead *head);
ListNode *ListNextItem(ListNode *node);
ListNode *ListGetItem(ListHead *head, int number);

// Overloads: ListHead and ListNode share the same layout,
// so the AUD_List.cpp functions accept ListHead* via implicit conversion.
static inline void ListNodeAppend(ListHead *head, ListNode *node) { ListNodeAppend((ListNode *)head, node); }
static inline void ListNodeInsert(ListHead *head, ListNode *node) { ListNodeInsert((ListNode *)head, node); }
static inline void ListNodeRemove(ListHead *head) { ListNodeRemove((ListNode *)head); }
static inline void *ListNodeNext(ListHead *head) { return ListNodeNext((ListNode *)head); }
static inline void *ListNodePrev(ListHead *head) { return ListNodePrev((ListNode *)head); }

#endif // _WPAUDIO_LIST_H_
