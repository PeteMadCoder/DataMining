#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

using PluginConfig = std::unordered_map<std::string, std::string>;

struct ProcessedData {
    std::string url;
    std::string title;
    std::string text_content;
    std::string html_content;
    std::vector<std::string> keywords;
    std::vector<std::string> links;
    std::vector<std::string> images;
    std::unordered_map<std::string, std::string> metadata; // Flexible key-value storage
    std::chrono::system_clock::time_point processed_time;
};

struct PluginMetadata {
    std::string name = "Unnamed Plugin";
    std::string version = "0.1.0";
    std::string description = "No description provided.";
    std::string author = "Unknown";
    // std::vector<std::string> dependencies; // Optional: list of required libraries/other plugins
};

// Base processor interface
class ContentProcessor {
public:
    virtual ~ContentProcessor() = default;
    virtual ProcessedData process(const std::string& url, const std::string& html_content) = 0;
    virtual std::string getName() const = 0;

    virtual void setConfig(const PluginConfig& config) {
        // Default implementation does nothing. Plugins can override.
        // This allows plugins to receive configuration without changing the core `process` signature.
    }

    // Get rich metadata about the plugin
    virtual PluginMetadata getMetadata() const {
        // Default implementation returns basic info.
        // Plugins should override to provide specific details.
        return PluginMetadata{name: this->getName()};
    }
};

// Processor registry
class ProcessorRegistry {
private:
    std::unordered_map<std::string, std::unique_ptr<ContentProcessor>> processors;

public:
    void registerProcessor(const std::string& name, std::unique_ptr<ContentProcessor> processor);
    ContentProcessor* getProcessor(const std::string& name);
    std::vector<std::string> getAvailableProcessors();
};