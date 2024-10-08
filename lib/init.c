#include "n77netincludes.h"

#if defined(_WIN32) || defined(_WIN64)
int socketInit() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void socketCleanup() {
    WSACleanup();
}
#else
int socketInit() {
    return 0;
}

void socketCleanup() {}
#endif