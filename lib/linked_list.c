#include <stdlib.h>
#include <assert.h>
#include "net77/linked_list.h"
#include "net77/thread_includes.h"

DataOwningLinkedListNode *newLinkedListNode(void *data) {
    DataOwningLinkedListNode *node = malloc(sizeof(DataOwningLinkedListNode));
    node->next = NULL;
    node->data = data;
    return node;
}

void linkedListNodeDestroy(DataOwningLinkedListNode *node) {
    free(node->data);
    free(node);
}

LinkedList newLinkedList() {
    LinkedList out = {.mutex = newMutex(), .start = NULL, .end = NULL};
    return out;
}

void linkedListDestroy(LinkedList *list) {
    mutexLock(&list->mutex);
    for (DataOwningLinkedListNode *next, *node = list->start; node != NULL; node = next) {
        next = node->next;
        linkedListNodeDestroy(node);
    }
    mutexUnlock(&list->mutex);
    mutexDestroy(&list->mutex);
    list->start = NULL;
    list->end = NULL;
}

size_t linkedListLen(LinkedList *list) {
    if (!list)
        return 0;
    mutexLock(&list->mutex);
    size_t len = 0;
    for (DataOwningLinkedListNode *node = list->start; node != NULL; node = node->next) {
        len++;
    }
    mutexUnlock(&list->mutex);
    return len;
}

void linkedListPushBack(LinkedList *list, void *data) {
    mutexLock(&list->mutex);
    if (!list->start || !list->end) {
        assert(!list->start && !list->end);
        DataOwningLinkedListNode *node = newLinkedListNode(data);
        list->start = node;
        list->end = node;
    } else {
        list->end->next = newLinkedListNode(data);
        list->end = list->end->next;
    }
    mutexUnlock(&list->mutex);
}

void *linkedListPopFront(LinkedList *list) {
    mutexLock(&list->mutex);
    void *data = NULL;
    if (!list->start || !list->end) {
        assert(!list->start && !list->end);
    } else {
        DataOwningLinkedListNode *node = list->start;
        data = node->data;
        if (list->start == list->end) {
            list->end = NULL;
            assert(!list->start->next);
        }
        list->start = list->start->next;
        free(node);
    }
    mutexUnlock(&list->mutex);
    return data;
}