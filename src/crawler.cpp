#include "crawler.h"
#include "downloader.h"
#include "parser.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Thread safety
std::mutex queue_mutex;
std::mutex visited_mutex;
std::atomic<int> downloaded_count{0};
std::atomic<bool> should_stop{false};


WebCrawler::WebCrawler(const std::string& start_url, const CrawlOptions& opts)
    : start_url(start_url), options(opts) 
{
    base_domain = Utils::extractBaseDomain(start_url);
    std::cout << "Base domain: " << base_domain << std::endl;
    url_queue.push(start_url);
    visited.insert(start_url);

    // Initialize the thread pool
    thread_pool = std::make_unique<ThreadPool>(options.concurrent_threads);
    
    // Reset atomic counters
    downloaded_count = 0;
    should_stop = false;
}

void WebCrawler::crawl() {
    const std::string outDir = "output";
    if (!Utils::createOutputDirectory(outDir)) {
        std::cerr << "Failed to create output directory" << std::endl;
        return;
    }


    // Create futures for all worker threads
    std::vector<std::future<void>> futures;
    
    // Launch worker threads
    for (int i = 0; i < options.concurrent_threads; ++i) {
        futures.emplace_back(
            thread_pool->enqueue([this]() {
                this->workerFunction();
            })
        );
    }

    // Wait for all threads to complete
    for (auto& future : futures) {
        try {
            future.wait();
        } catch (...) {
            // Handle exceptions if needed
        }
    }

    std::cout << "Crawled " << downloaded_count.load() << " pages" << std::endl;
}

bool WebCrawler::tryPopUrl(std::string& url) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (!url_queue.empty()) {
        url = url_queue.front();
        url_queue.pop();
        return true;
    }
    return false;
}

void WebCrawler::workerFunction() {
    while (!should_stop.load()) {
        // Check page limit
        if (options.max_pages != -1 && downloaded_count.load() >= options.max_pages) {
            should_stop.store(true);
            break;
        }

        std::string url;
        if (tryPopUrl(url)) {
            std::cout << "Downloading: " << url << std::endl;
            std::string html = Downloader::download(url);

            if (!html.empty()) {
                int current_count = downloaded_count.fetch_add(1) + 1;
                
                // Save HTML to file
                std::string safe_filename = Utils::createSafeFilename(url);
                std::string filename = options.output_dir + "/" + safe_filename + ".html";
                {
                    std::lock_guard<std::mutex> file_lock(queue_mutex); // Reuse mutex for file safety
                    std::ofstream outfile(filename, std::ios::binary);
                    outfile << html;
                    outfile.close();
                }
                
                // Extract links and add to queue (thread-safe)
                {
                    std::lock_guard<std::mutex> visited_lock(visited_mutex);
                    LinkParser::extractLinks(html, base_domain, url_queue, visited);
                }
                
                // Check if we've reached the limit
                if (options.max_pages != -1 && current_count >= options.max_pages) {
                    should_stop.store(true);
                    break;
                }
            } else {
                std::cout << "Failed to download: " << url << std::endl;
            }
        } else {
            // No URLs available, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}