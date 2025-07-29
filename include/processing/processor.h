#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>

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

// Base processor interface
class ContentProcessor {
public:
    virtual ~ContentProcessor() = default;
    virtual ProcessedData process(const std::string& url, const std::string& html_content) = 0;
    virtual std::string getName() const = 0;
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