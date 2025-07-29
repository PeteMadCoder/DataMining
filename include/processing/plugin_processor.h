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

public:
    PluginProcessor(const std::string& name) : processor_name(name) {}

    void addExtractor(ExtractorFunction extractor) {
        extractors.push_back(extractor);
    }

    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override {return processor_name;}
};