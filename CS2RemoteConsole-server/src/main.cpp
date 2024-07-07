#include "server.h"
#include "utils.h"
#include <iostream>
#include <csignal>

std::atomic<bool> applicationRunning(true);

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received. Initiating shutdown..." << std::endl;
    applicationRunning = false;
}

int main()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

#ifdef _WIN32
    if (!initializeWinsock())
    {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }
#endif

    Server server(42069, applicationRunning);
    if (!server.start())
    {
        return 1;
    }

    server.run();

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
