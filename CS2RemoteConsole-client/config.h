#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <memory>

#pragma once

class Config {
public:
    static Config& getInstance();
    
    void load(const std::string& filename);
    std::string get(const std::string& key, const std::string& default_value = "") const;
    int getInt(const std::string& key, int default_value = 0) const;

    // Delete copy constructor and assignment operator
    Config(Config const&) = delete;
    void operator=(Config const&) = delete;

private:
    Config() {} // Private constructor
    std::unordered_map<std::string, std::string> config_map;
};

#endif // CONFIG_H