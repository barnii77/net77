#ifndef NET77_MCFSS_H
#define NET77_MCFSS_H

#include <stddef.h>

typedef struct MultiCategoryFixedSizeSet {
    // category data (pointer to char arrays): char[n_categories][cap]
    char **data;
    // num bytes per item for each category: size_t[n_categories]
    size_t *item_sizes;
    // num categories. a category is a set of contiguous items of some item size associated with one item from each other category but not stored in memory next to the assoc items
    size_t n_categories;
    // shared category info
    size_t len;
    size_t cap;
} MultiCategoryFixedSizeSet;

MultiCategoryFixedSizeSet newMcfsSet(size_t cap, size_t *item_sizes, size_t n_categories);

/**
 * Adds to a set.
 * returns: 0 if item was added or was already present, 1 if item couldn't be added because the capacity was already maxed out
 */
int mcfsSetAdd(MultiCategoryFixedSizeSet *set, const char **items, size_t exist_check_category);

/**
 * Removes from a set.
 * returns: 0 if item was removed from set, 1 if it wasn't there in the first place
 */
int mcfsSetRemove(MultiCategoryFixedSizeSet *set, const char *item, size_t item_category);

char *mcfsSetGetAssocItemPtr(MultiCategoryFixedSizeSet *set, const char *item, size_t item_group,
                             size_t target_item_group);

/**
 * Destroys (deallocates) a set
 */
void mcfsSetDestroy(MultiCategoryFixedSizeSet *set);

#endif