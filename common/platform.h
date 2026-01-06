#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdlib.h>
    #pragma comment(lib, "ws2_32.lib")

    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define WOULD_BLOCK_ERROR WSAEWOULDBLOCK
    #define CLOSE_SOCKET closesocket
    #define SHUT_RDWR SD_BOTH

    inline bool platformSocketInit() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    inline void platformSocketCleanup() {
        WSACleanup();
    }

    inline void setSocketNonBlocking(SOCKET sock) {
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
    }

#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>

    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define SOCKET_ERROR_CODE errno
    #define WOULD_BLOCK_ERROR EWOULDBLOCK
    #define CLOSE_SOCKET close
    #define closesocket close

    inline bool platformSocketInit() {
        return true; // No initialization needed on POSIX
    }

    inline void platformSocketCleanup() {
        // No cleanup needed on POSIX
    }

    inline void setSocketNonBlocking(SOCKET sock) {
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

#endif

// Common utilities
#include <cstdint>

inline uint32_t byteSwap32(uint32_t value) {
#ifdef _WIN32
    return _byteswap_ulong(value);
#else
    return __builtin_bswap32(value);
#endif
}

#endif // PLATFORM_H
