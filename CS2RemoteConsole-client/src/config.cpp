#include "config.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

Config& Config::getInstance()
{
    static Config instance;
    return instance;
}

void Config::load(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("Unable to open config file: " + filename);
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '='))
        {
            std::string value;
            if (std::getline(is_line, value))
            {
                config_map[key] = value;
            }
        }
    }
}

std::string Config::get(const std::string& key, const std::string& default_value) const
{
    auto it = config_map.find(key);
    return (it != config_map.end()) ? it->second : default_value;
}

int Config::getInt(const std::string& key, int default_value) const
{
    auto it = config_map.find(key);
    if (it != config_map.end())
    {
        try
        {
            return std::stoi(it->second);
        }
        catch (...)
        {
            return default_value;
        }
    }
    return default_value;
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
            spdlog::info("[Config] Attempting to load config from: {}", path);
            Config::getInstance().load(path);
            spdlog::info("[Config] Config loaded successfully from: {}", path);
            return true;
        }
        catch (const std::exception& e)
        {
            spdlog::error("[Config] Failed to load config from: {}: {}", path, e.what());
        }
    }

    spdlog::error("[Config] Failed to load config from any location.");
    return false;
}
