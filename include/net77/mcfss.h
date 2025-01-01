#ifndef NET77_MCFSS_H
#define NET77_MCFSS_H

#include <stddef.h>

/**
 * A special data structure that allows conveniently decomposing an array of structs into a struct of arrays.
 * Specifically, this type represents a set, meaning conceptually, it does not care about in-memory item ordering.
 * This type is useful for when you really would want things to be grouped into a struct and managed as one piece,
 * but this is simply not possible because you need multiple arrays where the data that belongs together has the same
 * index in their respective arrays. In such cases, the MultiCategoryFixedSizeSet allows for joint management of the
 * associated items. \n For example: \n
 * ```c \n
 * struct pollfds pfds[] = {pfd1, pfd2, pfd3}; \n
 * int pfd_time_to_live[] = {99, 42, 1337}; \n
 * ``` \n
 * You have two arrays where items at the same index i in those arrays belong together conceptually, e.g. 42 is the time
 * to live of pfd2. With this type, you can add, remove and modify these pairs almost as if they were packed into a
 * struct, while still having them separated into different arrays in-memory.
 */
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
 * @return: 0 if item was added or was already present, 1 if item couldn't be added because the capacity was already maxed out
 */
int mcfsSetAdd(MultiCategoryFixedSizeSet *set, const char **items, size_t exist_check_category);

/**
 * Removes from a set.
 * @return: 0 if item was removed from set, 1 if it wasn't there in the first place
 */
int mcfsSetRemove(MultiCategoryFixedSizeSet *set, const char *item, size_t item_category);

char *mcfsSetGetAssocItemPtr(MultiCategoryFixedSizeSet *set, const char *item, size_t item_group,
                             size_t target_item_group);

/**
 * Destroys (deallocates) a set
 */
void mcfsSetDestroy(MultiCategoryFixedSizeSet *set);

#endif