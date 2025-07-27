#include "crawler.h"
#include "downloader.h"
#include "parser.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>


WebCrawler::WebCrawler(const std::string& start_url, const CrawlOptions& opts)
    : start_url(start_url), options(opts) 
{
    base_domain = Utils::extractBaseDomain(start_url);
    std::cout << "Base domain: " << base_domain << std::endl;
    url_queue.push(start_url);
    visited.insert(start_url);
}

void WebCrawler::crawl() {
    const std::string outDir = "output";
    if (!Utils::createOutputDirectory(outDir)) {
        std::cerr << "Failed to create output directory" << std::endl;
        return;
    }

    int count = 0;

    while (!url_queue.empty()) {
        if (options.max_pages != -1 && count >= options.max_pages) {
            std::cout << "Reached maximum page limit of " << options.max_pages << std::endl;
            break;
        }

        std::string url = url_queue.front();
        url_queue.pop();

        std::cout << "Downloading (" << (count + 1) << "): " << url << std::endl;
        std::string html = Downloader::download(url);

        if (!html.empty()) {
            count++;
            // Save HTML to file
            std::string safe_filename = Utils::createSafeFilename(url);
            std::string filename = options.output_dir + "/" + safe_filename + ".html";
            std::ofstream outfile(filename, std::ios::binary);
            outfile << html;
            outfile.close();
            
            // Extract links and add to queue
            LinkParser::extractLinks(html, base_domain, url_queue, visited);
        } else {
            std::cout << "Failed to download: " << url << std::endl;
        }

    }

    std::cout << "Crawled " << count << " pages" << std::endl;
}