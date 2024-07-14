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
//const std::string ANSI_COLOR_COMMON = "\033[38;5;251m";
//const std::string ANSI_COLOR_ID = "\033[38;5;220m";
//const std::string ANSI_COLOR_USERNAME = "\033[38;5;39m";
//const std::string ANSI_COLOR_IP_PORT = "\033[38;5;208m";
//const std::string ANSI_COLOR_GAME_IP = "\033[38;5;104m";
//const std::string ANSI_COLOR_GAME_PING = "\033[38;5;166m";
//const std::string ANSI_COLOR_TOTAL_LATENCY = "\033[38;5;72m";
//const std::string ANSI_COLOR_NETBUFFER = "\033[38;5;180m";
//const std::string ANSI_COLOR_SMOOTH = "\033[38;5;96m";
//const std::string ANSI_COLOR_TIMESTAMP = "\033[38;5;71m";
//const std::string ANSI_COLOR_RESET = "\033[0m";

const std::string ANSI_COLOR_COMMON = "\033[38;2;150;150;150m";
const std::string ANSI_COLOR_ID = "\033[38;2;160;120;0m";
const std::string ANSI_COLOR_USERNAME = "\033[38;2;25;120;210m";
const std::string ANSI_COLOR_IP_PORT = "\033[38;2;180;110;50m";
const std::string ANSI_COLOR_GAME_IP = "\033[38;2;100;80;200m";
const std::string ANSI_COLOR_GAME_PING = "\033[38;2;180;70;70m";
const std::string ANSI_COLOR_TOTAL_LATENCY = "\033[38;2;50;150;100m";
const std::string ANSI_COLOR_NETBUFFER = "\033[38;2;160;150;90m";
const std::string ANSI_COLOR_SMOOTH = "\033[38;2;120;90;180m";
const std::string ANSI_COLOR_TIMESTAMP = "\033[38;2;70;90;40m";


const std::string ANSI_COLOR_BACKGROUND_HEADER = "\033[48;2;30;00;50m";
const std::string ANSI_COLOR_COMMON_HEADER = "\033[38;2;200;200;200m";
const std::string ANSI_COLOR_ID_HEADER = "\033[38;2;220;180;0m";
const std::string ANSI_COLOR_USERNAME_HEADER = "\033[38;2;30;160;255m";
const std::string ANSI_COLOR_IP_PORT_HEADER = "\033[38;2;240;140;60m";
const std::string ANSI_COLOR_GAME_IP_HEADER = "\033[38;2;130;110;255m";
const std::string ANSI_COLOR_GAME_PING_HEADER = "\033[38;2;240;100;100m";
const std::string ANSI_COLOR_TOTAL_LATENCY_HEADER = "\033[38;2;70;200;130m";
const std::string ANSI_COLOR_NETBUFFER_HEADER = "\033[38;2;210;200;120m";
const std::string ANSI_COLOR_SMOOTH_HEADER = "\033[38;2;160;120;240m";
const std::string ANSI_COLOR_TIMESTAMP_HEADER = "\033[38;2;100;130;60m";

const std::string ANSI_COLOR_RESET = "\033[0m";

#ifdef _WIN32
bool initializeWinsock();
#endif

void setNonBlocking(SOCKET socket);
std::string getFormattedTime(const std::chrono::system_clock::time_point& time);

#endif // UTILS_H
