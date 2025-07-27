#pragma once
#include <string>
#include <queue>
#include <unordered_set>

struct CrawlOptions {
    int max_pages = -1;  // -1 means no limit
    std::string output_dir = "output";
};

class WebCrawler {
private:
    std::string start_url;
    std::string base_domain;
    std::queue<std::string> url_queue;
    std::unordered_set<std::string> visited;
    CrawlOptions options;
    
public:
    WebCrawler(const std::string& start_url, const CrawlOptions& opts = CrawlOptions());
    void crawl();
    std::string getBaseDomain() const {return base_domain;}
};