#include "query_system.h"
#include <algorithm>
#include <cctype>

TextSearchQuery::TextSearchQuery(const std::string& term, bool case_sensitive) 
    : search_term(term), case_sensitive(case_sensitive) {}

bool TextSearchQuery::matches(const ProcessedData& data) {
    std::string content = data.text_content;
    std::string title = data.title;
    std::string search = search_term;
    
    if (!case_sensitive) {
        // Convert to lowercase for case-insensitive search
        std::transform(content.begin(), content.end(), content.begin(), ::tolower);
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
    }
    
    return (content.find(search) != std::string::npos) || 
           (title.find(search) != std::string::npos);
}

RegexQuery::RegexQuery(const std::string& pattern_str) 
    : pattern(pattern_str) {}

bool RegexQuery::matches(const ProcessedData& data) {
    try {
        return std::regex_search(data.text_content, pattern) || 
               std::regex_search(data.title, pattern);
    } catch (...) {
        return false; // Invalid regex
    }
}

MetadataQuery::MetadataQuery(const std::string& metadata_key, const std::string& metadata_value)
    : key(metadata_key), value(metadata_value) {}

bool MetadataQuery::matches(const ProcessedData& data) {
    auto it = data.metadata.find(key);
    if (it != data.metadata.end()) {
        return it->second == value;
    }
    return false;
}

void AndQuery::addQuery(std::unique_ptr<DataQuery> query) {
    queries.push_back(std::move(query));
}

bool AndQuery::matches(const ProcessedData& data) {
    for (const auto& query : queries) {
        if (!query->matches(data)) {
            return false;
        }
    }
    return true;
}