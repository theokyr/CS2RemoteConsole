#ifndef TUI_H
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

struct ConsoleMessage
{
    std::string channel_name;
    std::string message;
    uint32_t color;
};


class TUI {
public:
    TUI();
    ~TUI();

    void init();
    void run();
    void shutdown();

    void setCommandCallback(std::function<void(const std::string&)> callback);
    void addLogMessage(const std::string& message);
    void addConsoleMessage(std::string channelName, std::string message, uint32_t color = 0);

    WINDOW* m_consoleWindow;

private:
    static const size_t MAX_LOG_MESSAGES = 1000;
    static const size_t MAX_CONSOLE_MESSAGES = 1000;

    WINDOW* m_logWindow;

    WINDOW* m_inputWindow;
    std::vector<std::string> m_logMessages;
    std::vector<ConsoleMessage> m_consoleMessages;

    std::string m_inputBuffer;
    std::function<void(const std::string&)> m_commandCallback;
    std::mutex m_logMutex;
    std::mutex m_consoleMutex;
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