﻿#ifndef TUI_H
#define TUI_H

#pragma comment(lib, "pdcurses.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "advapi32.lib")

#include <curses.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <unordered_map>

struct ConsoleChannel
{
    int id;
    std::string name;
    uint32_t color;
};

struct ConsoleMessage
{
    int channelId;
    std::string message;
};

class TUI
{
public:
    TUI();
    ~TUI();

    void init();
    void run();
    void shutdown();

    void setCommandCallback(std::function<void(const std::string&)> callback);
    void addLogMessage(const std::string& message);
    void addConsoleMessage(int channelId, const std::string& message);
    void registerChannel(int id, const std::string& name, uint32_t color);

    WINDOW* m_consoleWindow;

private:
    static const size_t MAX_LOG_MESSAGES = 1000;
    static const size_t MAX_CONSOLE_MESSAGES = 1000;

    WINDOW* m_logWindow;
    WINDOW* m_inputWindow;
    std::vector<std::string> m_logMessages;
    std::vector<ConsoleMessage> m_consoleMessages;
    std::unordered_map<int, ConsoleChannel> m_channels;

    std::string m_inputBuffer;
    std::function<void(const std::string&)> m_commandCallback;
    std::mutex m_logMutex;
    std::mutex m_consoleMutex;
    std::mutex m_channelsMutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_needsResize;
    int m_lastWidth;
    int m_lastHeight;

    void checkResize();
    void createWindows();
    void destroyWindows();
    void drawConsoleWindow();
    void drawInputWindow();
    void drawLogWindow();
    void drawWindows();
    void handleInput();
    void handleResize();
    void resizeWindows(int height, int width);
};

#endif // TUI_H
