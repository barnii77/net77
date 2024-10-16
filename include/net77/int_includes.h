#ifndef NET77_INT_INCLUDES_H
#define NET77_INT_INCLUDES_H

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

#endif