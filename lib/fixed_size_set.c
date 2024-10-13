#include <stdlib.h>
#include <memory.h>
#include "net77/fixed_size_set.h"

size_t findInSet(FixedSizeSet *set, const char *item) {
    for (size_t i = 0; i < set->len; i++)
        if (memcmp(&set->data[i], item, set->item_size) == 0)
            return i;
    return -1;
}

FixedSizeSet newFixedSizeSet(size_t cap, size_t item_size) {
    char *data = malloc(cap * item_size);
    FixedSizeSet set = {.data = data, .len = 0, .cap = cap, .item_size = item_size};
    return set;
}

int fixedSizeSetAdd(FixedSizeSet *set, const char *item) {
    if (set->len == set->cap)
        return 1;
    if (findInSet(set, item) == -1)
        memcpy(&set->data[set->len++], item, set->item_size);
    return 0;
}

int fixedSizeSetRemove(FixedSizeSet *set, const char *item) {
    size_t idx = findInSet(set, item);
    if (idx == -1)
        return 1;
    memmove(&set->data[idx], &set->data[idx + 1], --set->len - idx);
    return 0;
}

void fixedSizeSetDestroy(FixedSizeSet *set) {
    if (set->data)
        free(set->data);
    set->data = NULL;
    set->len = 0;
    set->cap = 0;
}