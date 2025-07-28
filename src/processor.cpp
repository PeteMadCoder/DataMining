#include "processor.h"
#include <gumbo.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

void ProcessorRegistry::registerProcessor(const std::string& name, std::unique_ptr<ContentProcessor> processor) {
    processors[name] = std::move(processor);
}

ContentProcessor* ProcessorRegistry::getProcessor(const std::string& name) {
    auto it = processors.find(name);
    if (it != processors.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> ProcessorRegistry::getAvailableProcessors() {
    std::vector<std::string> names;
    for (const auto& pair : processors) {
        names.push_back(pair.first);
    }
    return names;
}