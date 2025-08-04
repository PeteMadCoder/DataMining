#pragma once
#include <functional>
#include <unordered_map>
#include "processor.h"

// Plugin interface for easy extension
class PluginProcessor : public ContentProcessor {
public:
    using ExtractorFunction = std::function<void(const std::string& html, ProcessedData& data)>;

private:
    std::string processor_name;
    std::vector<ExtractorFunction> extractors;
    PluginConfig current_config;
    PluginMetadata plugin_metadata;

public:
    explicit PluginProcessor(const std::string& name, const PluginMetadata& metadata = PluginMetadata{})
        : processor_name(name), plugin_metadata(metadata) {
        // If no name was set in metadata, use the processor name
        if (plugin_metadata.name == "Unnamed Plugin") {
            plugin_metadata.name = name;
        }
    }

    void addExtractor(ExtractorFunction extractor) {
        extractors.push_back(extractor);
    }

    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override {return processor_name;}

    void setConfig(const PluginConfig& config) override {
        current_config = config;
    }

    PluginMetadata getMetadata() const override {
        return plugin_metadata;
    }

    const PluginConfig& getConfig() const { return current_config; }
};