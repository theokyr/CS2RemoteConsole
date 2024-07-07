#ifndef SERVER_H
#define SERVER_H

#include "client_handler.h"
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>

class Server
{
public:
    Server(uint16_t initialPort, std::atomic<bool>& running);
    ~Server();

    bool start();
    void run();
    uint16_t getCurrentPort() const { return m_port; }

private:
    uint16_t m_initialPort;
    uint16_t m_port;
    SOCKET m_listenSocket;
    std::vector<ClientInfo> m_clients;
    std::mutex m_clientsMutex;
    std::thread m_acceptThread;
    std::atomic<bool>& m_running;

    void acceptClients();
    void handleClient(ClientInfo& client);
    void handleClientMessage(ClientInfo& client, const std::string& message);
    void userInputHandler();
    void broadcastToClients(const std::string& message, const ClientInfo* excludeClient = nullptr);
    void removeClient(const ClientInfo& client);
    void cleanupSockets();
};

#endif // SERVER_H
