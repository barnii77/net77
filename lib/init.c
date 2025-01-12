#include "net77/net_includes.h"
#include "net77/type_utils.h"

#if defined(_WIN32) || defined(_WIN64)
ErrorStatus socketInit() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void socketCleanup() {
    WSACleanup();
}
#else

ErrorStatus socketInit() {
    return 0;
}

void socketCleanup() {}

#endif