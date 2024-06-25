#include "main.h"

// Global variables
SOCKET sock;
std::atomic<bool> running(true);
std::atomic<bool> listening(false);
std::thread listenerThread;
std::thread sanityCheckThread;

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
    closesocket(sock);
    WSACleanup();
}

int main()
{
    if (!setupConfig() || !setupWinsock())
    {
        return 1;
    }

    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);

    std::cout << "Using server IP: " << ip << " and port: " << port << '\n';

    if (!connectToServer(ip.c_str(), port))
    {
        cleanupWinsock();
        return 1;
    }

    bool debug_sanity = Config::getInstance().getInt("debug_sanity", 0) == 1;
    if (debug_sanity)
    {
        std::cout << "Debug sanity check enabled. Starting sanity check thread." << '\n';
        sanityCheckThread = std::thread(sendSanityCheck);
    }

    userInputHandler();

    running = false;
    if (debug_sanity && sanityCheckThread.joinable()) sanityCheckThread.join();
    if (listenerThread.joinable()) listenerThread.join();

    cleanupWinsock();
    return 0;
}

bool connectToServer(const char* ip, int port)
{
    const int cs2_console_reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);

    while (true)
    {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
        {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << '\n';
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);

        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection failed. Retrying in " << cs2_console_reconnect_delay / 1000 << " seconds..." << '\n';
            closesocket(sock);
            std::this_thread::sleep_for(std::chrono::milliseconds(cs2_console_reconnect_delay));
        }
        else
        {
            std::cout << "Connected to server " << ip << " on port " << port << '\n';
            return true;
        }
    }
}

int sendPayload(const std::vector<unsigned char>& payload)
{
    if (send(sock, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data" << '\n';
        return 1;
    }
    return 0;
}

void listenForData()
{
    char buffer[1024];
    int bytesReceived;
    u_long mode = 1; // Set non-blocking mode
    ioctlsocket(sock, FIONBIO, &mode);

    while (listening)
    {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            std::cout << "\nReceived: " << buffer << '\n' << ">> ";
        }
        else if (bytesReceived == 0)
        {
            std::cout << "\nConnection closed by server" << '\n';
            listening = false;
            running = false;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed: " << WSAGetLastError() << '\n';
            listening = false;
            running = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void userInputHandler()
{
    while (running)
    {
        std::cout << ">> ";
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        if (input.substr(0, 3) == "cmd")
        {
            if (input.length() > 4)
            {
                std::string command = input.substr(4);
                auto payload = create_command_payload(command);
                sendPayload(payload);
                std::cout << "Sent command: " << command << '\n';
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
                sendPayload(command_smooth_disable_payload);
                std::cout << "Sent smooth disable command" << '\n';
                break;
            case '1':
                sendPayload(command_smooth_enable_payload);
                std::cout << "Sent smooth enable command" << '\n';
                break;
            case 'y':
                if (!listening)
                {
                    listening = true;
                    listenerThread = std::thread(listenForData);
                    std::cout << "Started console output listening thread" << '\n';
                }
                else
                {
                    listening = false;
                    if (listenerThread.joinable()) listenerThread.join();
                    std::cout << "Stopped console output listening thread" << '\n';
                }
                break;
            case 'x':
                running = false;
                listening = false;
                return;
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
            sendPayload(command_say_sanity_check_payload);
            std::cout << "Sent sanity check command" << '\n';
        }
    }
}
