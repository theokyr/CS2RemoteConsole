#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <winsock2.h>

std::vector<unsigned char> create_payload(const std::vector<unsigned char>& bytes);
int send_payload(SOCKET sock, const std::vector<unsigned char>& payload);

#endif // UTILS_H
