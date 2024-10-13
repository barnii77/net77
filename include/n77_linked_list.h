#ifndef NET77_N77_LINKED_LIST_H
#define NET77_N77_LINKED_LIST_H

#include "n77_thread_includes.h"

/**
 * A node in a linked list that *takes ownership of the data*
 */
typedef struct DataOwningLinkedListNode {
    void *data;
    struct DataOwningLinkedListNode *next;
} DataOwningLinkedListNode;

DataOwningLinkedListNode *newLinkedListNode(void *data);

void linkedListNodeDestroy(DataOwningLinkedListNode *node);

/**
 * A thread safe linked list implementation
 */
typedef struct LinkedListHead {
    Mutex mutex;
    DataOwningLinkedListNode *start;
    DataOwningLinkedListNode *end;
} LinkedList;

LinkedList newLinkedList();

void linkedListDestroy(LinkedList *list);

/**
 * Pushes to the end of the linked list and *takes ownership of the data*
 */
void linkedListPushBack(LinkedList *list, void *data);

/**
 * Pops off the start of the linked list and *returns ownership of the data*
 */
void *linkedListPopFront(LinkedList *list);

#endif