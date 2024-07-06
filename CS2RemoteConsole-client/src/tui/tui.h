#ifndef TUI_H
#define TUI_H

#pragma comment(lib, "pdcurses.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "advapi32.lib")

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include <curses.h>
#include <spdlog/spdlog.h>

const int APPLICATION_SPECIAL_CHANNEL_ID = -1337;

struct ConsoleChannel
{
    std::string name;
    uint32_t color;
    short colorPairId;
};

struct ConsoleMessage
{
    int channelId;
    uint32_t color;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
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
    void addConsoleMessage(int channelId, const std::string& message, uint32_t msgColor = 0);
    void registerChannel(int id, const std::string& name, uint32_t color, uint32_t backgroundColor = 0);
    void setConsoleDirty(bool dirty);
    void setupLoggerCallbackSink();

private:
    static const size_t MAX_CONSOLE_MESSAGES = 10000;
    static const int MOUSE_SCROLL_SPEED = 3; // Scroll speed multiplier for mouse wheel
    static const int EXTENDED_COLOR_BASE = 256;

    std::unordered_map<long long, short> m_colorCache;
    short m_nextColorPairId = 1;
    int m_nextColorId = 1;
    bool m_useExtendedColors;

    WINDOW* m_consoleWindow;
    WINDOW* m_inputWindow;
    std::deque<ConsoleMessage> m_consoleMessages;
    std::unordered_map<int, ConsoleChannel> m_channels;

    std::string m_inputBuffer;
    std::function<void(const std::string&)> m_commandCallback;
    std::mutex m_consoleMutex;
    std::mutex m_channelsMutex;
    std::atomic<bool> m_running;
    std::atomic<bool> m_needsResize;
    int m_lastWidth;
    int m_lastHeight;
    size_t m_scrollPosition;
    std::atomic<bool> m_consoleDirty;
    std::chrono::steady_clock::time_point m_lastUpdate;

    void createWindows();
    void destroyWindows();
    void drawConsoleWindow();
    void drawInputWindow();
    void drawWindows();
    void updateInputWindow(); // New method to update only the input window
    void handleInput();
    void handleResize();
    void resizeWindows(int height, int width);
    void scrollConsole(int direction);

    short mapTo256Color(uint32_t color);
    short initializeColor(uint32_t color, uint32_t backgroundColor = 0);
};

#endif // TUI_H
