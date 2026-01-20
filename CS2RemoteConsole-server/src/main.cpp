#include "server.h"
#include "utils.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>
#include <cstdio>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

std::atomic<bool> applicationRunning(true);

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received. Initiating shutdown..." << std::endl;
    applicationRunning = false;
#ifdef _WIN32
    _close(_fileno(stdin));  // Force getline to return
#else
    close(STDIN_FILENO);  // Force getline to return
#endif
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    uint16_t port = 42069; // Default port

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if ((arg == "-p" || arg == "--port") && i + 1 < argc)
        {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
    }

#ifdef _WIN32
    if (!initializeWinsock())
    {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }
#endif

    Server server(port, applicationRunning);
    if (!server.start())
    {
        return 1;
    }

    server.run();

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
