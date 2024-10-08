#include <string.h>
#include "n77utils.h"

StringRef removeURLPrefix(StringRef url) {
    StringRef init = url;
    while (url.len > 0 && isLetter(url.data[0]))
        url.len--, url.data++;
    if (url.len > 0 && url.data[0] == ':')
        url.len--, url.data++;
    else
        return init;
    for (int i = 0; i < 2; i++) {
        if (url.len > 0 && url.data[0] == '/')
            url.len--, url.data++;
        else
            return init;
    }
    return url;
}

StringRef charPtrToStringRef(const char *data) {
    StringRef out = {strlen(data), data};
    return out;
}