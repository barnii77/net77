#ifndef NET77_STRING_UTILS_H
#define NET77_STRING_UTILS_H

#include <stddef.h>
#include "net77/type_utils.h"

/// reference to externally owned string, doesn't care about allocating/deallocating
typedef struct StringRef {
    size_t len;
    const char *nullable data;
} StringRef;

/// owned string, must be allocated/deallocated
typedef struct String {
    size_t len;
    char *nullable data;
} String;

/// continually build a string whose size is not initially known
typedef struct StringBuilder {
    size_t len;
    size_t cap;
    char *nullable data;
} StringBuilder;

void freeString(String *nonnull str);

StringBuilder newStringBuilder(size_t cap);

void stringBuilderExpandBuf(StringBuilder *nonnull builder, size_t min_new_size);

void stringBuilderAppend(StringBuilder *nonnull builder, const char *nonnull data, size_t len);

// allocates new memory for string and copies built string there
String stringBuilderBuild(StringBuilder *nonnull builder);

// resets the string builder and uses the string builders memory (after realloc to reduce capacity to exact required size)
String stringBuilderBuildAndDestroy(StringBuilder *nonnull builder);

void stringBuilderDestroy(StringBuilder *nonnull builder);

const char *nullable
findAsciiSubstringCaseInsensitive(const char *nonnull haystack, size_t haystack_len, const char *nonnull needle,
                                  size_t needle_len);

#endif