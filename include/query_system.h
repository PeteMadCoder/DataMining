#pragma once
#include "processor.h"
#include <regex>
#include <functional>

// Query interface for flexible searching
class DataQuery {
public:
    virtual ~DataQuery() = default;
    virtual bool matches(const ProcessedData& data) = 0;
};

// Text search query
class TextSearchQuery : public DataQuery {
private:
    std::string search_term;
    bool case_sensitive;
    
public:
    TextSearchQuery(const std::string& term, bool case_sensitive = false);
    bool matches(const ProcessedData& data) override;
};

// Regex search query
class RegexQuery : public DataQuery {
private:
    std::regex pattern;
    
public:
    RegexQuery(const std::string& pattern_str);
    bool matches(const ProcessedData& data) override;
};

// Metadata filter query
class MetadataQuery : public DataQuery {
private:
    std::string key;
    std::string value;
    
public:
    MetadataQuery(const std::string& metadata_key, const std::string& metadata_value);
    bool matches(const ProcessedData& data) override;
};

// Composite queries (AND, OR, NOT)
class AndQuery : public DataQuery {
private:
    std::vector<std::unique_ptr<DataQuery>> queries;
    
public:
    void addQuery(std::unique_ptr<DataQuery> query);
    bool matches(const ProcessedData& data) override;
};