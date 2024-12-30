#include <stdio.h>

#ifndef NET77_LOGGING_H
#define NET77_LOGGING_H

#ifdef NET77_ENABLE_LOGGING
#define LOG_MSG(msg, ...) {printf(msg, ##__VA_ARGS__); fflush(stdout);}
#else
#define LOG_MSG(msg, ...) {}
#endif

#endif