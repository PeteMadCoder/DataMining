#include "plugin_processor.h"
#include <gumbo.h>
#include <chrono>

ProcessedData PluginProcessor::process(const std::string& url, const std::string& html_content) {
    ProcessedData data;
    data.url = url;
    data.html_content = html_content;
    data.processed_time = std::chrono::system_clock::now();

    // Run all registered extractors
    for (const auto& extractor : extractors) {
        extractor(html_content, data);
    }

    return data;
}