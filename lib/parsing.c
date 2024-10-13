#include <stdlib.h>
#include <string.h>
#include "net77/serde.h"

#define STR_EQ_LITERAL(expr, literal) (!strncmp((expr), (literal), sizeof(literal) - 1))

#define STR_LOWER_EQ_LITERAL(expr, literal) (!strLowerCmp((expr), (literal), sizeof(literal) - 1))

#define BUBBLE_UP_ERR(err) if (err) return 1

int strToInt(const char *data, size_t len) {
    int out = 0;
    for (size_t i = 0; i < len; i++)
        out = out * 10 + data[i] - '0';
    return out;
}

int expectSpace(StringRef *str) {
    if (str->len == 0 || str->data[0] != ' ')
        return 1;
    str->data++;
    str->len--;
    return 0;
}

int expectNewline(StringRef *str) {
    if (str->len <= 1 || str->data[0] != '\r' || str->data[1] != '\n')
        return 1;
    str->data += 2;
    str->len -= 2;
    return 0;
}

int expectChar(StringRef *str, char c) {
    if (str->len == 0 || str->data[0] != c)
        return 1;
    str->data++;
    str->len--;
    return 0;
}

char toLower(char c) {
    char off = (char) (('A' <= c && c <= 'Z') * ('a' - 'A'));
    return c + off;
}

int strLowerCmp(const char *s1, const char *s2, int len) {
    for (int i = 0; i < len; i++) {
        char a = toLower(s1[i]), b = toLower(s2[i]);
        if (a != b || a == '\0' || b == '\0')
            return a < b ? -1 : a == b ? 0 : 1;
    }
    return 0;
}

int isLetter(char c) {
    return 'A' <= c && c <= 'Z' || 'a' <= c && c <= 'z';
}

int isDigit(char c) {
    return '0' <= c && c <= '9';
}

StringRef parseWord(StringRef *str) {
    for (int i = 0; i < str->len; i++) {
        char c = str->data[i];
        if (!isLetter(c)) {
            str->len -= i;
            str->data += i;
            StringRef out = {i, str->data - i};
            return out;
        }
    }
    StringRef out = {str->len, str->data};
    return out;
}

StringRef parseNumber(StringRef *str) {
    for (int i = 0; i < str->len; i++) {
        char c = str->data[i];
        if (!isDigit(c)) {
            str->len -= i;
            str->data += i;
            StringRef out = {i, str->data - i};
            return out;
        }
    }
    StringRef out = {str->len, str->data};
    return out;
}

int parseMethod(StringRef *str, Method *method) {
    StringRef ms = parseWord(str);
    if (STR_EQ_LITERAL(ms.data, "GET")) {
        *method = METHOD_GET;
    } else if (STR_EQ_LITERAL(ms.data, "POST")) {
        *method = METHOD_POST;
    } else if (STR_EQ_LITERAL(ms.data, "HEAD")) {
        *method = METHOD_HEAD;
    } else {
        return 1;
    }
    return 0;
}

int parseURL(StringRef *str, StringRef *url) {
    int i;
    for (i = 0; i < str->len; i++) {
        if (str->data[i] == ' ') {
            break;
        }
    }
    str->len -= i;
    str->data += i;
    StringRef out = {i, str->data - i};
    *url = out;
    return i == 0;
}

int parseVersion(StringRef *str, Version *v) {
    StringRef http = parseWord(str);
    if (!STR_LOWER_EQ_LITERAL(http.data, "http"))
        return 1;

    BUBBLE_UP_ERR(expectChar(str, '/'));
    StringRef major = parseNumber(str);
    if (major.len > 1 || major.data[0] != '0' && major.data[0] != '1' && major.data[0] != '2')
        return 1;
    int maj = major.data[0] - '0';

    BUBBLE_UP_ERR(expectChar(str, '.'));
    StringRef minor = parseNumber(str);
    int min = minor.data[0] - '0';

    if (maj == 0)
        *v = VERSION_HTTP10;  // use oldest supported version
    else if (maj == 1)
        *v = min == 0 ? VERSION_HTTP10 : VERSION_HTTP11;
    else
        *v = VERSION_HTTP11;  // use newest supported version

    return 0;
}

int parseHeaderField(StringRef *str, HeaderField *h) {
    StringRef name = parseWord(str);
    BUBBLE_UP_ERR(expectChar(str, ':'));
    while (str->len > 0 && str->data[0] == ' ') {
        str->len--;
        str->data++;
    }
    const char *start = str->data;
    while (str->len > 1 && (str->data[0] != '\r' || str->data[1] != '\n')) {
        str->data++;
        str->len--;
    }
    const char *end = str->data;
    while (--str->len > 0 && (++str->data)[-1] != '\n');
    if (str->len == 0)
        return 1;  // missing final empty line to terminate http header and mark start of optional body
    StringRef value = {end - start, start};
    HeaderField out = {name, value};
    *h = out;
    return 0;
}

int parseHeaderStruct(StringRef *str, HeaderStruct *hs) {
    int err, count = 0, last_was_nl = 0, is_beginning = 1;
    for (int i = 0; i < str->len - 1; i++) {
        if (str->data[i] == '\r' && str->data[i + 1] == '\n') {
            if (is_beginning) {
                hs->count = 0;
                hs->fields = NULL;
                return 0;
            }
            if (last_was_nl)
                break;
            last_was_nl = 1;
            count++, i++;
        } else {
            last_was_nl = 0;
        }
        is_beginning = 0;
    }

    int hfsize = count * (int) sizeof(HeaderField);
    HeaderField *hf = malloc(hfsize);
    if (!hf)
        return 1;
    for (int i = 0; i < count; i++) {
        err = parseHeaderField(str, &hf[i]);
        if (err) {
            memset(hf, 0, hfsize);
            free(hf);
            return err;
        }
    }

    if (str->len <= 1 || str->data[0] != '\r' || str->data[1] != '\n') {
        memset(hf, 0, hfsize);
        free(hf);
        return 1;
    }
    hs->count = count;
    hs->fields = hf;
    return 0;
}

int parseHeaderString(StringRef *str, StringRef *h) {
    const char *start = str->data;
    int last_was_nl = 0, is_beginning = 0;
    for (;; str->data++, str->len--) {
        if (str->len > 1 && str->data[0] == '\r' || str->data[1] == '\n') {
            if (is_beginning) {
                h->data = start;
                h->len = 0;
                return 0;
            }
            if (last_was_nl)
                break;
            str->data++;
            str->len--;
            last_was_nl = 1;
        } else {
            last_was_nl = 0;
        }
        is_beginning = 1;
    }
    while (str->len > 0 && (++str->data)[-1] != '\n')
        str->len--;
    if (str->len == 0)
        return 1;  // missing final empty line to terminate http header and mark start of optional body

    str->len++;
    str->data += 2;
    h->len = str->data - start;
    h->data = start;
    return 0;
}

int parseHeader(StringRef *str, Header *head, int parse_header_into_structs) {
    if (parse_header_into_structs) {
        head->type = HEADER_AS_STRUCT;
        HeaderStruct hs;
        BUBBLE_UP_ERR(parseHeaderStruct(str, &hs));
        HeaderData data = {.structure = hs};
        head->data = data;
    } else {
        head->type = HEADER_AS_STRING;
        StringRef h;
        BUBBLE_UP_ERR(parseHeaderString(str, &h));
        HeaderData data = {.string = h};
        head->data = data;
    }
    return 0;
}

int parseBody(StringRef *str, StringRef *body) {
    body->data = str->data;
    body->len = str->len;
    return 0;
}

int parseRequest(StringRef str, Request *req, int parse_header_into_structs) {
    Method method;
    BUBBLE_UP_ERR(parseMethod(&str, &method));
    BUBBLE_UP_ERR(expectSpace(&str));

    StringRef url;
    BUBBLE_UP_ERR(parseURL(&str, &url));
    BUBBLE_UP_ERR(expectSpace(&str));

    Version version;
    BUBBLE_UP_ERR(parseVersion(&str, &version));
    BUBBLE_UP_ERR(expectNewline(&str));

    Header head;
    BUBBLE_UP_ERR(parseHeader(&str, &head, parse_header_into_structs));
    BUBBLE_UP_ERR(expectNewline(&str));

    StringRef body;
    BUBBLE_UP_ERR(parseBody(&str, &body));

    req->method = method;
    req->url = url;
    req->version = version;
    req->head = head;
    req->body = body;

    return 0;
}

int parseResponse(StringRef str, Response *resp, int parse_header_into_structs) {
    Version version;
    BUBBLE_UP_ERR(parseVersion(&str, &version));
    BUBBLE_UP_ERR(expectSpace(&str));

    StringRef status_code_str = parseNumber(&str);
    if (status_code_str.len != 3 || status_code_str.data[0] <= '0' || status_code_str.data[0] > '5')
        return 1;  // invalid status code
    int status_code = strToInt(status_code_str.data, status_code_str.len);
    BUBBLE_UP_ERR(expectSpace(&str));

    StringRef status_msg = {str.len, str.data};
    while (str.len > 1 && (str.data[0] != '\r' || str.data[1] != '\n')) {
        str.data++;
        str.len--;
    }
    status_msg.len -= str.len;
    BUBBLE_UP_ERR(expectNewline(&str));

    Header head;
    BUBBLE_UP_ERR(parseHeader(&str, &head, parse_header_into_structs));
    BUBBLE_UP_ERR(expectNewline(&str));

    StringRef body;
    BUBBLE_UP_ERR(parseBody(&str, &body));

    resp->version = version;
    resp->status_code = status_code;
    resp->status_msg = status_msg;
    resp->head = head;
    resp->body = body;

    return 0;
}