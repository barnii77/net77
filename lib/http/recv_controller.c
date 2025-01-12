#include <string.h>
#include <assert.h>
#include "net77/logging.h"
#include "net77/net_includes.h"
#include "net77/type_utils.h"
#include "net77/math_utils.h"
#include "net77/string_utils.h"
#include "net77/http/serde.h"

// this has to be 1 for 100% correctness
#define HTTP_RECV_CONTROLLER_INIT_BATCH_RECV_SIZE 1
static const char content_length_header_name[] = "Content-Length:";
static const char header_field_delimiter[] = "\r\n";
static const char end_of_header[] = "\r\n\r\n";

#define strlenof(s) (sizeof(s) - sizeof(char))

/**
 * @param begin where to start parsing the base 10 number from
 * @param end where to stop parsing
 * @param error_on_illegal_char if non-zero, the function will return -1 when a non-ascii-digit character is hit.
 * If zero, the function will consider this the end of the number and return the number *so far*. However, if no
 * legal characters have been encountered yet, it will *still return -1*.
 * @param out_terminator if not null, the character that terminated the number (if any) will be written out
 * @return -1 on error, otherwise parsed number
 */
static ssize_t strToUint(const char *nonnull begin, const char *nonnull end, char *nullable out_terminator) {
    assert(begin && end && begin <= end);
    ssize_t out = 0;
    bool has_found_legal = false;
    for (const char *p = begin; p < end; p++) {
        char c = *p;
        if ('0' <= c && c <= '9') {
            out = out * 10 + c - '0';
            has_found_legal = true;
        } else if (c == ' ' || c == '\t') {
            continue;
        } else if (!has_found_legal) {
            if (out_terminator)
                *out_terminator = c;
            return -1;
        } else {
            break;
        }
    }
    return out;
}

static ssize_t getHeaderSize(const char *nonnull header_begin, const size_t n) {
    const char *eoh = findAsciiSubstringCaseInsensitive(header_begin, n, end_of_header, strlenof(end_of_header));
    if (eoh)
        return (ssize_t) (eoh - header_begin + strlenof(end_of_header));
    else
        return -1;
}

typedef struct HttpReceiveControllerState {
    ssize_t og_max_len;
    ssize_t content_length;
    HttpVersion http_version;
    int phase;
    bool is_cte;
} HttpReceiveControllerState;

ErrorStatus parseHttpVersion(StringRef *nonnull str, HttpVersion *nonnull v);

// TODO handle chunked transfer encoding
void httpReceiveController(const char *nonnull buf, size_t cap, size_t n, ssize_t *nonnull max_len,
                           RecvAllDataControllerCommandedAction *next_action, void *nonnull opaque_controller_state) {
    LOG_MSG("called http recv controller having read %zu bytes (max_len = %zd)", n, *max_len);
    HttpReceiveControllerState *controller_state = opaque_controller_state;
    assert(buf && max_len && controller_state);
    switch (controller_state->http_version * 10 + controller_state->phase) {
        case 0: {
            LOG_MSG("(http recv controller) running phase 0 - initialize");
            assert(!n);  // first call must be before first recv
            controller_state->http_version = 0;
            controller_state->phase++;
            controller_state->og_max_len = *max_len;
            *max_len = optMin((ssize_t) n + HTTP_RECV_CONTROLLER_INIT_BATCH_RECV_SIZE, controller_state->og_max_len);
            break;
        }
        case 1: {
            LOG_MSG("(http recv controller) running phase 1 - recv first line to parse version");
            *max_len = optMin((ssize_t) n + HTTP_RECV_CONTROLLER_INIT_BATCH_RECV_SIZE, controller_state->og_max_len);
            const char *next_line = findAsciiSubstringCaseInsensitive(buf, n, header_field_delimiter,
                                                                      strlenof(header_field_delimiter));
            if (!next_line) {
                LOG_MSG("(http recv controller) version not yet found");
                return;
            }
            // first line complete - can extract version
            controller_state->http_version = VERSION_HTTP09;
            StringRef s = {.data = buf, .len = n};
            if (parseHttpVersion(&s, &controller_state->http_version)) {
                *next_action = RECV_ALL_DATA_ACTION_REJECT_REQUEST;
                return;
            }
            LOG_MSG("(http recv controller) detected HTTP version %d.%d", controller_state->http_version / 10,
                    controller_state->http_version % 10);
            controller_state->phase++;
            if (controller_state->http_version == VERSION_HTTP09 || controller_state->http_version == VERSION_HTTP10) {
                LOG_MSG("Doing full receive until connection close");
                *max_len = controller_state->og_max_len;
            }
            break;
        }
        case 112: {
            LOG_MSG("(http recv controller) entered phase 2 in HTTP 1.1 mode - receive carefully until end of header");
            size_t offset = n - HTTP_RECV_CONTROLLER_INIT_BATCH_RECV_SIZE - strlenof(end_of_header) + 1;
            if (n < strlenof(end_of_header) || getHeaderSize(buf + offset, n - offset) < 0) {
                *max_len = optMin((ssize_t) n + HTTP_RECV_CONTROLLER_INIT_BATCH_RECV_SIZE,
                                  controller_state->og_max_len);
                return;
            } else {
                controller_state->phase++;
            }
        }
        case 113: {
            // header complete
            const char *line = findAsciiSubstringCaseInsensitive(buf, n, content_length_header_name,
                                                                 strlenof(content_length_header_name));
            if (!line) {
                // content length header field does not exist
                *next_action = RECV_ALL_DATA_ACTION_REJECT_REQUEST;
                return;
            }
            line += strlenof(content_length_header_name);
            char terminator = '\r';
            const ssize_t content_length = strToUint(line, buf + n, &terminator);
            if (content_length >= 0 && terminator == '\r')
                controller_state->content_length = content_length;
            else
                // field value incomplete (wait for more data)
                return;

            // determine where the header ends
            ssize_t header_size = getHeaderSize(buf, n);
            if (header_size >= 0) {
                // n_overshot_bytes = how many bytes of the body have already been batch-read
                ssize_t n_overshot_bytes = (ssize_t) n - header_size;
                *max_len = optMin(content_length + header_size - n_overshot_bytes, controller_state->og_max_len);
                controller_state->phase = 999999;
            } else {
                controller_state->phase++;
            }
            break;
        }
        case 114: {
            ssize_t header_size = getHeaderSize(buf, n);
            if (header_size >= 0) {
                *max_len = optMin(controller_state->content_length + header_size, controller_state->og_max_len);
                controller_state->phase++;
            } else {
                *next_action = RECV_ALL_DATA_ACTION_REJECT_REQUEST;
            }
            break;
        }
    }
}

RecvAllDataControllerCallback http_recv_controller = {.fn = httpReceiveController, .sizeof_state = sizeof(HttpReceiveControllerState), .shared_state = NULL};

//static void toFmt(char c, char out[static 4]) {
//    switch (c) {
//        case '\n':
//            out[0] = '\\';
//            out[1] = 'n';
//            out[2] = '\n';
//            break;
//        case '\r':
//            out[0] = '\\';
//            out[1] = 'r';
//            out[2] = '\r';
//            break;
//        default:
//            out[0] = c;
//    }
//}
//
//static void printTransformedStr(const char *s, void (*transform)(char c, char out[static 3])) {
//    for (; *s != '\0'; s++) {
//        char out[4] = {'\0'};
//        transform(*s, out);
//        printf("%s", out);
//    }
//    printf("\n");
//}
