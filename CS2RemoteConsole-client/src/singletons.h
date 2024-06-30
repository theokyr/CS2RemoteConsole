#ifndef SINGLETONS_H
#define SINGLETONS_H

#pragma once

#include "../libvconsole/vconsole.h"
#include <memory>
#include <mutex>

class VConsoleSingleton {
public:
    static VConsole& getInstance() {
        static VConsole instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    VConsoleSingleton(const VConsoleSingleton&) = delete;
    VConsoleSingleton& operator=(const VConsoleSingleton&) = delete;

private:
    VConsoleSingleton() = default;
    ~VConsoleSingleton() = default;
};

#endif
