#include "connection_cs2console.h"
#include "spdlog/spdlog.h"
#include "../config.h"
#include <chrono>
#include <thread>

std::atomic<bool> listeningCS2(false);
std::atomic<bool> cs2ConsoleConnected(false);
std::atomic<bool> running(true);
std::thread cs2ConnectorThread;
std::thread cs2ListenerThread;

bool connectToCS2Console()
{
    auto& vconsole = VConsoleSingleton::getInstance();
    const std::string ip = Config::getInstance().get("cs2_console_ip", "127.0.0.1");
    const int port = Config::getInstance().getInt("cs2_console_port", 29000);

    if (!vconsole.connect(ip, port))
    {
        return false;
    }

    cs2ConsoleConnected = true;
    spdlog::info("[CS2ConsoleConnection] Connected to CS2 console at {}:{}", ip, port);
    return true;
}

void cs2ConsoleConnectorLoop()
{
    auto& vconsole = VConsoleSingleton::getInstance();
    const int reconnect_delay = Config::getInstance().getInt("cs2_console_reconnect_delay", 5000);
    const int sanity_check_interval = Config::getInstance().getInt("debug_sanity_interval", 5000);

    bool debug_sanity = Config::getInstance().getInt("debug_sanity_enabled", 0) == 1;
    if (debug_sanity)
    {
        spdlog::info("[CS2ConsoleConnection] Debug sanity check enabled!");
    }

    auto last_sanity_check = std::chrono::steady_clock::now();

    while (running)
    {
        if (!cs2ConsoleConnected)
        {
            if (connectToCS2Console())
            {
                cs2ConsoleConnected = true;
                listeningCS2 = true;
                if (cs2ListenerThread.joinable())
                {
                    cs2ListenerThread.join();
                }
                cs2ListenerThread = std::thread(listenForCS2ConsoleData);
                vconsole.sendCmd("name"); // Get current player's in-game name
            }
            else
            {
                spdlog::error("[CS2ConsoleConnection] Failed to connect to CS2 console. Retrying in {} seconds...", reconnect_delay / 1000);
                for (int i = 0; i < reconnect_delay / 100 && running; ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }
        }

        if (debug_sanity && cs2ConsoleConnected)
        {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sanity_check).count() >= sanity_check_interval)
            {
                if (!vconsole.sendCmd("echo CS2RemoteConsole_SanityCheck"))
                {
                    spdlog::warn("[CS2ConsoleConnection] Failed to send sanity check command. Connection may be unstable.");
                }
                else
                {
                    spdlog::debug("[CS2ConsoleConnection] Sent sanity check command to CS2 console");
                }
                last_sanity_check = now;
            }
        }

        // Check if the connection is still alive
        if (!vconsole.isConnected())
        {
            spdlog::error("[CS2ConsoleConnection] Connection to CS2 console lost. Attempting to reconnect...");
            cs2ConsoleConnected = false;
            listeningCS2 = false;
            vconsole.disconnect();
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void listenForCS2ConsoleData()
{
    auto& vconsole = VConsoleSingleton::getInstance();

    while (listeningCS2 && running)
    {
        try
        {
            if (!vconsole.processIncomingData())
            {
                if (!vconsole.isConnected())
                {
                    spdlog::error("[CS2ConsoleConnection] Connection lost while processing data.");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("[CS2ConsoleConnection] Exception in processIncomingData: {}", e.what());
            break;
        }
    }

    spdlog::info("[CS2ConsoleConnection] CS2 console listener thread stopping...");
    cs2ConsoleConnected = false;
    listeningCS2 = false;
}

int sendPayloadToCS2Console(const std::string& payload)
{
    auto& vconsole = VConsoleSingleton::getInstance();

    if (!cs2ConsoleConnected)
    {
        spdlog::error("[CS2ConsoleConnection] Cannot send payload: Not connected to CS2 console");
        return 1;
    }

    return vconsole.sendCmd(payload);
}

void cleanupCS2Console()
{
    auto& vconsole = VConsoleSingleton::getInstance();
    running = false;
    listeningCS2 = false;
    cs2ConsoleConnected = false;

    if (cs2ListenerThread.joinable())
    {
        cs2ListenerThread.join();
    }
    if (cs2ConnectorThread.joinable())
    {
        cs2ConnectorThread.join();
    }

    vconsole.disconnect();
}

void initializeCS2Connection()
{
    auto& vconsole = VConsoleSingleton::getInstance();

    vconsole.setOnCVARsLoaded([](const std::vector<Cvar>& cvars)
    {
        for (const auto& cvar : cvars)
        {
            spdlog::info("[CS2ConsoleConnection] CVAR loaded: {}", cvar.name);
        }
    });

    vconsole.setOnADONReceived([](const std::string& adonName)
    {
        spdlog::info("[CS2ConsoleConnection] ADON: {}", adonName);
    });

    vconsole.setOnDisconnected([]()
    {
        spdlog::warn("[CS2ConsoleConnection] Disconnected from CS2 console");
        cs2ConsoleConnected = false;
        listeningCS2 = false;
    });

    vconsole.setOnPRNTReceived([](const PRNT& prnt)
    {
        if (prnt.message.find("CS2RemoteConsole_SanityCheck") != std::string::npos)
        {
            spdlog::debug("[CS2ConsoleConnection] Received sanity check response");
        }
    });

    cs2ConnectorThread = std::thread(cs2ConsoleConnectorLoop);
}
