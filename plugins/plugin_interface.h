#pragma once
#include "processor.h"
#include <memory>

extern "C" {
    typedef void (*RegisterPluginFunction)(ProcessorRegistry&);

    // Functions to get plugin information
    typedef const char* (*GetPluginNameFunction)();
    typedef const char* (*GetPluginVersionFunction)();
    typedef const char* (*GetPluginDescriptionFunction)();
}

// Plugin loader class
class PluginLoader {
public:
    static bool loadPlugin(const std::string& plugin_path, ProcessorRegistry& registry);
    static std::vector<std::string> findPlugins(const std::string& plugins_directory);
};