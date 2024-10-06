#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "n77serde.h"

#ifdef _MSC_VER
#include <intrin.h>
#define _builtin_clz __lzcnt64
#else
#define _builtin_clz __builtin_clz
#define max(a, b) ((a < b) ? (b) : (a))
#endif

StringBuilder newStringBuilder(size_t cap) {
    if (cap == 0) {
        StringBuilder out = {.len = 0, .cap = 0, .data = NULL};
        return out;
    } else {
        char *data = malloc(cap);
        if (!data)
            assert(0 && "failed to allocate memory for StringBuilder");
        StringBuilder out = {.len = 0, .cap = cap, .data = data};
        return out;
    }
}

void stringBuilderExpandBuf(StringBuilder *builder, size_t min_new_size) {
    assert(min_new_size > builder->cap &&
           "when expanding the buffer of a StringBuilder, the minimum new size must be bigger than the capacity of the buffer");
    if (!min_new_size)
        return;
    // new cap is next power of 2 after min_new_size or 64 (to skip a bunch of tiny allocations)
    size_t new_cap = max(64, 1 << (8 * sizeof(size_t) - _builtin_clz(min_new_size - 1)));
    char *buf = realloc(builder->data, new_cap);
    if (!buf) {
        free(builder->data);
        assert(0 && "failed to allocate memory for StringBuilder");
    }
    builder->data = buf;
    builder->cap = new_cap;
}

void stringBuilderAppend(StringBuilder *builder, const char *data, size_t len) {
    size_t new_len = builder->len + len;
    if (builder->cap < new_len)
        stringBuilderExpandBuf(builder, new_len);
    memcpy(&builder->data[builder->len], data, len);
    builder->len = new_len;
}

// allocates new memory for string and copies built string there
String stringBuilderBuild(StringBuilder *builder) {
    char *data = malloc(builder->len + 1);
    String out = {.len = builder->len, .data = data};
    memcpy(data, builder->data, out.len);
    data[out.len] = 0;
    return out;
}

// resets the string builder and uses the string builders memory (after realloc to reduce capacity to exact required size)
String stringBuilderBuildAndDestroy(StringBuilder *builder) {
    char *data = realloc(builder->data, builder->len + 1);
    if (!data) {
        free(builder->data);
        assert(0 && "failed to realloc builder data for final build");
    }
    String out = {.len = builder->len, .data = data};
    data[out.len] = 0;
    builder->data = NULL;
    builder->cap = 0;
    builder->len = 0;
    return out;
}