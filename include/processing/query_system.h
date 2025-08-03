#pragma once
#include "processor.h"
#include <regex>
#include <functional>
#include <string>
#include <vector>
#include <memory>

// Abstract base class for all queries
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
    std::string patter_str;
    
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

// URL Regex filter
class UrlRegexQuery : public DataQuery {
private:
    std::regex url_pattern;

public:
    UrlRegexQuery(const std::string& pattern_str);
    bool matches(const ProcessedData& data) override;
};

// Composite queries: matches only if ALL sub-queries match (AND logic)
class AndQuery : public DataQuery {
private:
    std::vector<std::unique_ptr<DataQuery>> queries;
    
public:
    void addQuery(std::unique_ptr<DataQuery> query);
    bool matches(const ProcessedData& data) override;
};

// Composite queries: matches only if one sub-queries match (OR logic)
class OrQuery : public DataQuery {
private:
    std::vector<std::unique_ptr<DataQuery>> queries;
    
public:
    void addQuery(std::unique_ptr<DataQuery> query);
    bool matches(const ProcessedData& data) override;
};

// Decorator query: negates the result of another query (NOT logic)
class NotQuery : public DataQuery {
private:
    std::unique_ptr<DataQuery> query;

public:
    NotQuery(std::unique_ptr<DataQuery> q);
    bool matches(const ProcessedData& data) override;
};