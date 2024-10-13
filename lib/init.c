#include "n77_net_includes.h"

#ifdef _MSC_VER
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