#include "plugin_interface.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE PluginHandle;
    #define LoadPlugin LoadLibraryA
    #define GetPluginSymbol GetProcAddress
    #define ClosePlugin FreeLibrary
    #define GetPluginError() "Windows error"
#else
    #include <dlfcn.h>
    typedef void* PluginHandle;
    #define LoadPlugin(filename) dlopen(filename, RTLD_LAZY)
    #define GetPluginSymbol(handle, symbol) dlsym(handle, symbol)
    #define ClosePlugin dlclose
    #define GetPluginError() dlerror()
#endif

static bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool PluginLoader::loadPlugin(const std::string& plugin_path, ProcessorRegistry& registry) {
    std::cout << "Loading plugin: " << plugin_path << std::endl;

    // Load shared library
    PluginHandle handle = LoadPlugin(plugin_path.c_str());
    if (!handle) {
        std::cerr << "Failed to load plugin " << plugin_path << ": " << GetPluginError() << std::endl;
        return false;
    }

    // Get the registration function
    RegisterPluginFunction register_func = (RegisterPluginFunction)GetPluginSymbol(handle, "registerPlugin");

    if (!register_func) {
        std::cerr << "Plugin " << plugin_path << " does not export 'registerPlugin' function" << std::endl;

#ifdef _WIN32
        ClosePlugin(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    // Call the registration function
    try {
        register_func(registry);
        std::cout << "Successfully registered plugin from " << plugin_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error registering plugin " << plugin_path << ": " << e.what() << std::endl;
#ifdef _WIN32
        ClosePlugin(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    return true;
}


std::vector<std::string> PluginLoader::findPlugins(const std::string& plugins_directory) {
    std::vector<std::string> plugin_paths;

    if (!std::filesystem::exists(plugins_directory)) {
        std::cerr << "Plugins directory does not exist: " << plugins_directory << std::endl;
        return plugin_paths;
    }

    // Look for shared libraries in the plugins directory
    for (const auto& entry : std::filesystem::directory_iterator(plugins_directory)) {
        std::string path = entry.path().string();
        
#ifdef _WIN32
        if (entry.is_regular_file() && endsWith(path, ".dll")) {
            plugin_paths.push_back(path);
        }
#else
        // Check for both .so and .dylib extensions
        if (entry.is_regular_file() && (endsWith(path, ".so") || endsWith(path, ".dylib"))) {
            plugin_paths.push_back(path);
        }
#endif

    }
    
    return plugin_paths;

}