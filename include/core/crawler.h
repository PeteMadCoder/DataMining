#pragma once
#include <string>
#include <queue>
#include <unordered_set>
#include <queue>
#include <unordered_set>
#include <memory>
#include "thread_pool.h"

struct CrawlOptions {
    int max_pages = -1;  // -1 means no limit
    std::string output_dir = "output";
    int concurrent_threads = 5;     // Number of concurrent downloads
};

class WebCrawler {
private:
    std::string start_url;
    std::string base_domain;
    std::queue<std::string> url_queue;
    std::unordered_set<std::string> visited;
    CrawlOptions options;
    std::unique_ptr<ThreadPool> thread_pool;

    bool tryPopUrl(std::string& url);
    void workerFunction();
    
public:
    WebCrawler(const std::string& start_url, const CrawlOptions& opts = CrawlOptions());
    void crawl();
    std::string getBaseDomain() const {return base_domain;}
};