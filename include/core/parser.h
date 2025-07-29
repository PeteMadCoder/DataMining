#pragma once
#include <string>
#include <queue>
#include <unordered_set>

class LinkParser {
public:
    static void extractLinks(const std::string& html,
        const std::string& base_url,
        std::queue<std::string>& url_queue,
        std::unordered_set<std::string>& visited
    );
};