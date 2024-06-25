#include "main.h"

// Global variables
SOCKET cs2ConsoleSock = INVALID_SOCKET;
SOCKET remoteServerSock = INVALID_SOCKET;
std::atomic<bool> running(true);
std::atomic<bool> listeningCS2(false);
std::atomic<bool> listeningRemoteServer(false);
std::atomic<bool> remoteServerConnected(false);
std::thread cs2ListenerThread;
std::thread remoteServerListenerThread;
std::thread remoteServerConnectorThread;
std::thread sanityCheckThread;

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    running = false;
}

bool setupConfig()
{
    std::vector<std::string> config_paths = {
        "config.ini",
        getCurrentDirectory() + "\\config.ini"
    };

    for (const auto& path : config_paths)
    {
        try
        {
            std::cout << "Attempting to load config from: " << path << '\n';
            Config::getInstance().load(path);
            std::cout << "Config loaded successfully from: " << path << '\n';
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to load config from " << path << ": " << e.what() << '\n';
        }
    }

    std::cerr << "Failed to load config from any location." << '\n';
    return false;
}

bool setupWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << '\n';
        return false;
    }
    return true;
}

void cleanupWinsock()
{
    if (cs2ConsoleSock != INVALID_SOCKET)
        closesocket(cs2ConsoleSock);
    if (remoteServerSock != INVALID_SOCKET)
        closesocket(remoteServerSock);
    WSACleanup();
}

bool connectToCS2Console()
{
    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);
    const int reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);

    while (running)
    {
        cs2ConsoleSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (cs2ConsoleSock == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket for CS2 console: " << WSAGetLastError() << '\n';
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (connect(cs2ConsoleSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection to CS2 console failed. Retrying in " << reconnect_delay / 1000 << " seconds...\n";
            closesocket(cs2ConsoleSock);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
        }
        else
        {
            std::cout << "Connected to CS2 console at " << ip << ":" << port << '\n';
            return true;
        }

        if (!running)
        {
            std::cout << "Stopping CS2 console connection attempts due to shutdown request.\n";
            return false;
        }
    }
    return false;
}

bool connectToRemoteServer()
{
    const std::string ip = Config::getInstance().get("remote_server_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("remote_server_port", 42069);

    remoteServerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteServerSock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket for remote server: " << WSAGetLastError() << '\n';
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(remoteServerSock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection to remote server failed: " << WSAGetLastError() << '\n';
        closesocket(remoteServerSock);
        remoteServerSock = INVALID_SOCKET;
        return false;
    }

    std::cout << "Connected to remote server at " << ip << ":" << port << '\n';
    return true;
}

void remoteServerConnectorLoop()
{
    const int reconnect_delay = Config::getInstance().getInt("remote_server_reconnect_delay", 5000);

    while (running && !remoteServerConnected)
    {
        if (connectToRemoteServer())
        {
            remoteServerConnected = true;
            listeningRemoteServer = true;
            remoteServerListenerThread = std::thread(listenForRemoteServerData);
        }
        else
        {
            std::cout << "Failed to connect to remote server. Retrying in " << reconnect_delay / 1000 << " seconds...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
        }
    }
}

void listenForCS2ConsoleData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(cs2ConsoleSock, FIONBIO, &mode);

    while (listeningCS2 && running)
    {
        bytesReceived = recv(cs2ConsoleSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "\nReceived from CS2 console: " << buffer << '\n' << ">> ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by CS2 console" << '\n';
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed from CS2 console: " << WSAGetLastError() << '\n';
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "CS2 console listener thread stopping..." << '\n';
}

void listenForRemoteServerData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(remoteServerSock, FIONBIO, &mode);

    while (listeningRemoteServer && running)
    {
        bytesReceived = recv(remoteServerSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "\nReceived from remote server: " << buffer << '\n' << ">> ";
            // Forward the command to CS2 console
            auto payload = create_command_payload(buffer);
            sendPayload(cs2ConsoleSock, payload);
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by remote server" << '\n';
            break;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed from remote server: " << WSAGetLastError() << '\n';
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Remote server listener thread stopping..." << '\n';
    remoteServerConnected = false;
    listeningRemoteServer = false;
    closesocket(remoteServerSock);
    remoteServerSock = INVALID_SOCKET;

    // Restart the connector thread
    remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);
}

int sendPayload(SOCKET sock, const std::vector<unsigned char>& payload)
{
    if (send(sock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data" << '\n';
        return 1;
    }
    return 0;
}

void userInputHandler()
{
    while (running)
    {
        std::cout << ">> ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input == "quit" || input == "exit" || input == "x")
        {
            std::cout << "Exit command received. Initiating shutdown..." << '\n';
            running = false;
            break;
        }

        if (input.substr(0, 3) == "cmd")
        {
            if (input.length() > 4)
            {
                std::string command = input.substr(4);
                auto payload = create_command_payload(command);
                sendPayload(cs2ConsoleSock, payload);
                std::cout << "Sent command to CS2 console: " << command << '\n';
            }
            else
            {
                std::cout << "Invalid command format. Use 'cmd <your_command>'" << '\n';
            }
        }
        else
        {
            switch (input[0])
            {
            case '0':
                sendPayload(cs2ConsoleSock, command_smooth_disable_payload);
                std::cout << "Sent smooth disable command" << '\n';
                break;
            case '1':
                sendPayload(cs2ConsoleSock, command_smooth_enable_payload);
                std::cout << "Sent smooth enable command" << '\n';
                break;
            case 'y':
                if (!listeningCS2)
                {
                    listeningCS2 = true;
                    cs2ListenerThread = std::thread(listenForCS2ConsoleData);
                    std::cout << "Started CS2 console output listening thread" << '\n';
                }
                else
                {
                    listeningCS2 = false;
                    if (cs2ListenerThread.joinable()) cs2ListenerThread.join();
                    std::cout << "Stopped CS2 console output listening thread" << '\n';
                }
                break;
            default:
                std::cout << "Unknown command" << '\n';
                break;
            }
        }
    }
}

void sendSanityCheck()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (running)
        {
            sendPayload(cs2ConsoleSock, command_say_sanity_check_payload);
            std::cout << "Sent sanity check command to CS2 console" << '\n';
        }
    }
    std::cout << "Sanity check thread stopping..." << '\n';
}

void gracefulShutdown()
{
    std::cout << "Initiating graceful shutdown..." << '\n';

    running = false;
    listeningCS2 = false;
    listeningRemoteServer = false;

    if (sanityCheckThread.joinable())
    {
        std::cout << "Waiting for sanity check thread to finish..." << '\n';
        sanityCheckThread.join();
    }

    if (cs2ListenerThread.joinable())
    {
        std::cout << "Stopping CS2 console listener thread..." << '\n';
        cs2ListenerThread.join();
    }

    if (remoteServerListenerThread.joinable())
    {
        std::cout << "Stopping remote server listener thread..." << '\n';
        remoteServerListenerThread.join();
    }

    if (remoteServerConnectorThread.joinable())
    {
        std::cout << "Stopping remote server connector thread..." << '\n';
        remoteServerConnectorThread.join();
    }

    cleanupWinsock();

    std::cout << "Graceful shutdown complete." << '\n';
}

int main()
{
    signal(SIGINT, signalHandler);

    if (!setupConfig() || !setupWinsock())
    {
        return 1;
    }

    if (!connectToCS2Console())
    {
        cleanupWinsock();
        return 1;
    }

    // Start the remote server connector thread
    remoteServerConnectorThread = std::thread(remoteServerConnectorLoop);

    bool debug_sanity = Config::getInstance().getInt("debug_sanity", 0) == 1;
    if (debug_sanity)
    {
        std::cout << "Debug sanity check enabled. Starting sanity check thread." << '\n';
        sanityCheckThread = std::thread(sendSanityCheck);
    }

    userInputHandler();

    gracefulShutdown();
    return 0;
}