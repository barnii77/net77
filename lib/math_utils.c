#include "net77/int_includes.h"
#include "net77/math_utils.h"

ssize_t optMin(ssize_t a, ssize_t b) {
    if (a < 0 && b < 0)
        return -1;
    else if (a < 0)
        return b;
    else if (b < 0)
        return a;
    else
        return a < b ? a : b;
}

ssize_t optMax(ssize_t a, ssize_t b) {
    if (a < 0 && b < 0)
        return -1;
    else if (a < 0)
        return a;
    else if (b < 0)
        return b;
    else
        return a > b ? a : b;
}
