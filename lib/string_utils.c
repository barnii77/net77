#include <stddef.h>
#include <ctype.h>
#include "net77/type_utils.h"

const char *
findAsciiSubstringCaseInsensitive(const char *nonnull haystack, size_t haystack_len, const char *nonnull needle,
                                  size_t needle_len) {
    if (needle_len == 0)
        return haystack;

    if (haystack_len < needle_len)
        return NULL;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        size_t j;
        for (j = 0; j < needle_len; j++) {
            if (tolower((unsigned char) haystack[i + j]) != tolower((unsigned char) needle[j]))
                break;
        }
        if (j == needle_len)
            return haystack + i;
    }
    return NULL;
}