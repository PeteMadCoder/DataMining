#pragma once
#include "processor.h"

// Generic HTML processor with configurable extraction
class GenericProcessor : public ContentProcessor {
public:
    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override {return "generic";}
};

// Text-focused processor
class TextProcessor : public ContentProcessor {
public:
    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override { return "text"; }
};

// Metadata-focused processor
class MetadataProcessor : public ContentProcessor {
public:
    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override { return "metadata"; }
};

// Link analysis processor
class LinkProcessor : public ContentProcessor {
public:
    ProcessedData process(const std::string& url, const std::string& html_content) override;
    std::string getName() const override { return "links"; }
};