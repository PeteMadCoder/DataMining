#include "query_system.h"
#include <algorithm>
#include <cctype>
#include <iostream>

// --- TextSearchQuery ---
TextSearchQuery::TextSearchQuery(const std::string& term, bool case_sensitive) 
    : search_term(term), case_sensitive(case_sensitive) {}

bool TextSearchQuery::matches(const ProcessedData& data) {
    std::string content_to_search = data.title + " " + data.text_content;
    
    std::string haystack = content_to_search;
    std::string needle = search_term;

    if (!case_sensitive) {
        // Convert both to lowercase for comparison
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
        std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);
    }

    // Use std::string::find for substring search
    return haystack.find(needle) != std::string::npos;
}

// --- RegexQuery ---
RegexQuery::RegexQuery(const std::string& pattern_str) 
    : pattern(pattern_str, std::regex_constants::ECMAScript | std::regex_constants::optimize) {}

bool RegexQuery::matches(const ProcessedData& data) {
    try {
        // Search in title first, then text_content
        // Using std::regex_constants::ECMAScript is the default, but being explicit is good
        return std::regex_search(data.title, pattern) || 
               std::regex_search(data.text_content, pattern);
        // Optionally, you could search in html_content or other fields too
        // || std::regex_search(data.html_content, pattern);
    } catch (const std::regex_error& e) {
        // It's better to catch specific exceptions
        std::cerr << "RegexQuery error: Invalid regex '" << "" << "': " << e.what() << std::endl;
        return false; // Invalid regex means no match
    }
}

// --- MetadataQuery ---
MetadataQuery::MetadataQuery(const std::string& metadata_key, const std::string& metadata_value)
    : key(metadata_key), value(metadata_value) {}

bool MetadataQuery::matches(const ProcessedData& data) {
    auto it = data.metadata.find(key);
    if (it != data.metadata.end()) {
        return it->second == value;
    }
    return false;
}

// --- UrlRegexQuery ---
UrlRegexQuery::UrlRegexQuery(const std::string& pattern_str)
    // It's good practice to use ECMAScript and optimize flags
    : url_pattern(pattern_str, std::regex_constants::ECMAScript | std::regex_constants::optimize) {
    // The constructor body is empty as initialization is done in the member initializer list
    // If the pattern_str is invalid, constructing std::regex will throw std::regex_error
}

bool UrlRegexQuery::matches(const ProcessedData& data) {
    try {
        // Use std::regex_search to check if the URL contains a match for the pattern
        // Use std::regex_match if you want the entire URL to match the pattern
        // For URL prefix matching like '^https://www.olx.pt/d/anuncio/', search is appropriate
        // as the pattern would need to start with ^ if anchoring is desired.
        return std::regex_search(data.url, url_pattern);
    } catch (const std::regex_error& e) {
        // Handle potential errors during matching (less likely than construction error,
        // but good practice). Log and fail safe.
        std::cerr << "UrlRegexQuery error: Failed to match URL '" << data.url
                  << "' against pattern. Error: " << e.what() << std::endl;
        return false; // If matching fails, assume no match
    }
}

// --- AndQuery ---
void AndQuery::addQuery(std::unique_ptr<DataQuery> query) {
    if (query) { // Check for null pointer
        queries.push_back(std::move(query));
    }
}

bool AndQuery::matches(const ProcessedData& data) {
    for (const auto& query : queries) {
        if (!query->matches(data)) {
            return false; // If any sub-query fails, the AND query fails
        }
    }
    return true; // All sub-queries passed
}

// --- OrQuery ---
void OrQuery::addQuery(std::unique_ptr<DataQuery> query) {
    if (query) { // Check for null pointer
        queries.push_back(std::move(query));
    }
}

bool OrQuery::matches(const ProcessedData& data) {
    // If there are no sub-queries, it's an empty OR, which we can define as false or true.
    // False is often more logical (can't match anything).
    if (queries.empty()) {
        return false;
    }
    
    for (const auto& query : queries) {
        if (query->matches(data)) {
            return true; // If any sub-query succeeds, the OR query succeeds
        }
    }
    return false; // No sub-queries matched
}

// --- NotQuery ---
NotQuery::NotQuery(std::unique_ptr<DataQuery> q) : query(std::move(q)) {
    // The constructor now takes ownership of the query
}

bool NotQuery::matches(const ProcessedData& data) {
    if (!query) {
        // If there's no sub-query, NOT nothing could be considered true or false.
        // Let's say NOT nothing is false (cannot evaluate).
        return false;
    }
    // Return the opposite of the sub-query's result
    return !query->matches(data);
}