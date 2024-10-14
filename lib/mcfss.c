#include <stdlib.h>
#include <string.h>
#include "net77/mcfss.h"

#ifndef NET77_FIXED_SIZE_SET_H
#define NET77_FIXED_SIZE_SET_H

static size_t findInArray(const char *data, size_t len, const char *item, size_t item_size) {
    for (size_t i = 0; i < len; i++)
        if (memcmp(&data[i * item_size], item, item_size) == 0)
            return i;
    return -1;
}

MultiCategoryFixedSizeSet newMcfsSet(size_t cap, size_t *item_sizes, size_t n_categories) {
    char **data = malloc(n_categories * sizeof(char *));
    for (size_t cat = 0; cat < n_categories; cat++)
        data[cat] = malloc(cap * item_sizes[cat]);
    size_t *heap_alloc_item_sizes = malloc(n_categories * sizeof(size_t));
    memcpy(heap_alloc_item_sizes, item_sizes, n_categories * sizeof(size_t));
    MultiCategoryFixedSizeSet set = {
            .data = data,
            .len = 0,
            .cap = cap,
            .item_sizes = heap_alloc_item_sizes,
            .n_categories = n_categories
    };
    return set;
}

int mcfsSetAdd(MultiCategoryFixedSizeSet *set, const char **items, size_t exist_check_category) {
    if (set->len == set->cap)
        return 1;
    if (findInArray(set->data[exist_check_category], set->len, items[exist_check_category],
                    set->item_sizes[exist_check_category]) == -1) {
        for (size_t cat = 0; cat < set->n_categories; cat++)
            memcpy(&set->data[cat][set->len * set->item_sizes[cat]], items[cat], set->item_sizes[cat]);
        set->len++;
    }
    return 0;
}

int mcfsSetRemove(MultiCategoryFixedSizeSet *set, const char *item, size_t item_category) {
    size_t idx = findInArray(set->data[item_category], set->len, item, set->item_sizes[item_category]);
    if (idx == -1)
        return 1;
    for (size_t cat = 0; cat < set->n_categories; cat++)
        memmove(&set->data[cat][set->item_sizes[cat] * idx], &set->data[cat][set->item_sizes[cat] * (idx + 1)],
                set->item_sizes[cat] * (set->len - 1 - idx));
    set->len--;
    return 0;
}

char *mcfsSetGetAssocItemPtr(MultiCategoryFixedSizeSet *set, const char *item, size_t item_group,
                             size_t target_item_group) {
    size_t idx = findInArray(set->data[item_group], set->len, item, set->item_sizes[item_group]);
    if (idx == -1)
        return NULL;
    return set->data[target_item_group] + idx * set->item_sizes[target_item_group];
}

void mcfsSetDestroy(MultiCategoryFixedSizeSet *set) {
    for (size_t cat = 0; cat < set->n_categories; cat++) {
        free(set->data[cat]);
    }
    free(set->data);
    free(set->item_sizes);
    set->data = NULL;
    set->len = 0;
    set->cap = 0;
}

#endif