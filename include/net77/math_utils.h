#ifndef NET77_MATH_UTILS_H
#define NET77_MATH_UTILS_H

/// return the minimum of either numbers, but negative values encode positive infinity
ssize_t optMin(ssize_t a, ssize_t b);

/// return the maximum of either numbers, but negative values encode positive infinity
ssize_t optMax(ssize_t a, ssize_t b);

#define CEIL_DIV(x, to_multiple_of) (((x) + (to_multiple_of) - 1) / (to_multiple_of))

#endif