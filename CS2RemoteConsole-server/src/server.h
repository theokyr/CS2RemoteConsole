#ifndef SERVER_H
#define SERVER_H

#include "client_handler.h"
#include <atomic>
#include <thread>
#include <vector>

class Server
{
public:
    Server(uint16_t port, std::atomic<bool>& running);
    ~Server();

    bool start();
    void run();

private:
    uint16_t m_port;
    SOCKET m_listenSocket;
    std::vector<ClientInfo> m_clients;
    std::mutex m_clientsMutex;
    std::thread m_acceptThread;
    std::atomic<bool>& m_running;

    void acceptClients();
    void userInputHandler();
    void broadcastToClients(const std::string& message);
    void cleanupSockets();
};

#endif // SERVER_H
