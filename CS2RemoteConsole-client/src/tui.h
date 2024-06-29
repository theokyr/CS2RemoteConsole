#ifndef TUI_H
#define TUI_H

#pragma comment(lib, "pdcurses_win_x64_wide_utf8.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "advapi32.lib")

#include <curses.h>
#include <string>
#include <vector>
#include <functional>

#pragma once

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
    void addUserCommand(const std::string& command);
    void setOnPRNTReceivedCallback(std::function<void(const std::string&, const std::string&)> callback);

private:
    WINDOW* m_logWindow;
    WINDOW* m_inputWindow;
    std::vector<std::string> m_logMessages;
    std::string m_inputBuffer;
    std::function<void(const std::string&)> m_commandCallback;
    std::function<void(const std::string&, const std::string&)> m_prntReceivedCallback;

    void createWindows();
    void destroyWindows();
    void drawLogWindow();
    void drawInputWindow();
    void handleInput();
};

#endif
