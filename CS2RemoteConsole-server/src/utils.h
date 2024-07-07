#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define SOCKET_ERROR_CODE WSAGetLastError()
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define SOCKET_ERROR_CODE errno
#endif

// ANSI color code constants
const std::string ANSI_COLOR_COMMON = "\033[38;5;251m";
const std::string ANSI_COLOR_ID = "\033[38;5;220m";
const std::string ANSI_COLOR_USERNAME = "\033[38;5;39m";
const std::string ANSI_COLOR_IP_PORT = "\033[38;5;208m";
const std::string ANSI_COLOR_TIMESTAMP = "\033[38;5;71m";
const std::string ANSI_COLOR_RESET = "\033[0m"; 

#ifdef _WIN32
bool initializeWinsock();
#endif

void setNonBlocking(SOCKET socket);
std::string getFormattedTime(const std::chrono::system_clock::time_point& time);

#endif // UTILS_H
