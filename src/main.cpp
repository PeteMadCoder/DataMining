#include "crawler.h"
#include <iostream>
#include <curl/curl.h>
#include <string>
#include <cstdlib>

struct CrawlerOptions {
    std::string url;
    int max_pages = -1;     // -1 means no limit
    std::string output_dir = "output";
    bool help = false;
};

CrawlerOptions parseArguments(int argc, char** argv) {
    CrawlerOptions options;

    for (int i = 1; i<argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.help = true;
        }
        else if (arg == "--url" || arg == "-u") {
            if (i+1 < argc) {
                options.url = argv[++i];
            }
        }
        else if (arg == "--max-pages" || arg == "-m") {
            if (i + 1 < argc) {
                options.max_pages = std::atoi(argv[++i]);
            }
        }
        else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                options.output_dir = argv[++i];
            }
        }
        else if (options.url.empty() && arg.find("http") == 0) {
            // Assume first non-option argument starting with http is the URL
            options.url = arg;
        }
    }

    return options;
}

void printHelp(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [URL]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -u, --url URL           Starting URL to crawl\n";
    std::cout << "  -m, --max-pages N       Maximum number of pages to crawl (default: unlimited)\n";
    std::cout << "  -o, --output DIR        Output directory (default: output)\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " https://example.com\n";
    std::cout << "  " << program_name << " --url https://example.com --max-pages 50\n";
    std::cout << "  " << program_name << " -u https://example.com -m 100 -d 3\n";
}

int main(int argc, char** argv) {
    CrawlerOptions options = parseArguments(argc, argv);

    if (options.help) {
        printHelp(argv[0]);
        return 0;
    }

    if (options.url.empty()) {
        std::cerr << "Error: URL is required\n";
        printHelp(argv[0]);
        return 1;
    }

    std::cout << "Starting crawl with options:\n";
    std::cout << "  URL: " << options.url << "\n";
    std::cout << "  Max pages: " << (options.max_pages == -1 ? "unlimited" : std::to_string(options.max_pages)) << "\n";
    std::cout << "  Output dir: " << options.output_dir << "\n";
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    CrawlOptions crawl_opts;
    crawl_opts.max_pages = options.max_pages;
    crawl_opts.output_dir = options.output_dir;
    
    WebCrawler crawler(options.url, crawl_opts);
    crawler.crawl();
    
    curl_global_cleanup();
    return 0;
}