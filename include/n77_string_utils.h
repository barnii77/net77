#ifndef NET77_N77_STRING_UTILS_H
#define NET77_N77_STRING_UTILS_H

#include <stddef.h>

/// reference to externally owned string, doesn't care about allocating/deallocating
typedef struct StringRef {
    size_t len;
    const char *data;
} StringRef;

/// owned string, must be allocated/deallocated
typedef struct String {
    size_t len;
    char *data;
} String;

/// continually build a string whose size is not initially known
typedef struct StringBuilder {
    size_t len;
    size_t cap;
    char *data;
} StringBuilder;

void freeString(String *str);

StringBuilder newStringBuilder(size_t cap);

void stringBuilderExpandBuf(StringBuilder *builder, size_t min_new_size);

void stringBuilderAppend(StringBuilder *builder, const char *data, size_t len);

// allocates new memory for string and copies built string there
String stringBuilderBuild(StringBuilder *builder);

// resets the string builder and uses the string builders memory (after realloc to reduce capacity to exact required size)
String stringBuilderBuildAndDestroy(StringBuilder *builder);

void stringBuilderDestroy(StringBuilder *builder);

#endif