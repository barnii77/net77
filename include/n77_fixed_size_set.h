#ifndef NET77_N77_FIXED_SIZE_SET_H
#define NET77_N77_FIXED_SIZE_SET_H

#include <stddef.h>

typedef struct FixedSizeSet {
    char *data;
    size_t item_size;
    size_t len;
    size_t cap;
} FixedSizeSet;

FixedSizeSet newFixedSizeSet(size_t cap, size_t item_size);

/**
 * Adds to a set
 * @param set the set
 * @param item the bytes of the item
 * @return 0 if item was added or was already present, 1 if item couldn't be added because the capacity was already maxed out
 */
int fixedSizeSetAdd(FixedSizeSet *set, const char *item);

/**
 * Removes from a set
 * @param set the set
 * @param item the bytes of the item
 * @return 0 if item was removed from set, 1 if it wasn't there in the first place
 */
int fixedSizeSetRemove(FixedSizeSet *set, const char *item);

/**
 * Destroys (deallocates) a set
 * @param set the set
 * @param item the item
 */
void fixedSizeSetDestroy(FixedSizeSet *set);

#endif