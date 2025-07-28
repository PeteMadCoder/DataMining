#include "crawler.h"
#include "processing_pipeline.h"
#include "builtin_processors.h"
#include <iostream>
#include <curl/curl.h>
#include <string>
#include <cstdlib>

struct CrawlerOptions {
    // Crawler options
    std::string url;
    int max_pages = -1;     // -1 means no limit
    std::string output_dir = "output";
    int concurrent_threads = 5;

    // Processing options
    std::string input_dir;  // For processing existing files
    std::string processor_mode = "crawl";   // crawl, process, both
    std::string processor_type = "generic";
    std::string search_query;
    std::string export_format = "json";
    std::string export_file = "processed_output.json";

    bool help = false;
};

CrawlerOptions parseArguments(int argc, char** argv) {
    CrawlerOptions options;

    for (int i = 1; i<argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.help = true;
        }

        // Crawler options
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
        else if (arg == "--concurrent-threads" || arg == "-t") {
            if (i + 1 < argc) {
                options.concurrent_threads = std::atoi(argv[++i]);
            }
        }

        // Processor options
        else if (arg == "--process" || arg == "-p") {
            options.processor_mode = "process";
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                options.input_dir = argv[++i];
            }
        }
        else if (arg == "--both" || arg == "-b") {
            options.processor_mode = "both";
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                options.url = argv[++i];
            }
        }
        else if (arg == "--processor-type") {
            if (i + 1 < argc) {
                options.processor_type = argv[++i];
            }
        }
        else if (arg == "--query" || arg == "-q") {
            if (i + 1 < argc) {
                options.search_query = argv[++i];
            }
        }
        else if (arg == "--export" || arg == "-e") {
            if (i + 1 < argc) {
                options.export_format = argv[++i];
            }
        }
        else if (arg == "--export-file") {
            if (i + 1 < argc) {
                options.export_file = argv[++i];
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
    std::cout << "Usage: " << program_name << " [MODE] [OPTIONS]\n";
    std::cout << "\nModes:\n";
    std::cout << "  --url URL, -u URL      Crawl mode - start crawling from URL\n";
    std::cout << "  --process DIR, -p DIR  Process mode - process HTML files in directory\n";
    std::cout << "  --both URL, -b URL     Both mode - crawl then process\n";
    std::cout << "\nCrawler Options:\n";
    std::cout << "  -m, --max-pages N      Maximum number of pages to crawl (default: unlimited)\n";
    std::cout << "  -o, --output DIR       Output directory for crawled files (default: output)\n";
    std::cout << "  -t, --concurrent-threads N  Number of concurrent threads (default: 5)\n";
    std::cout << "\nProcessor Options:\n";
    std::cout << "  --processor-type TYPE  Processor type (generic, text, metadata, links)\n";
    std::cout << "  -q, --query TERM       Search query for filtering\n";
    std::cout << "  -e, --export FORMAT    Export format (json, csv, database)\n";
    std::cout << "  --export-file FILE     Output file name (default: processed_output.json)\n";
    std::cout << "\nGeneral Options:\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " --url https://example.com\n";
    std::cout << "  " << program_name << " --process ./output --processor-type text\n";
    std::cout << "  " << program_name << " --both https://example.com --max-pages 50\n";
    std::cout << "  " << program_name << " --process ./output --query \"Wikipedia\" --export csv --export-file results.csv\n";
}

int main(int argc, char** argv) {
    CrawlerOptions options = parseArguments(argc, argv);

    if (options.help) {
        printHelp(argv[0]);
        return 0;
    }

    if (options.processor_mode == "crawl" || options.processor_mode == "both") {

        if (options.url.empty()) {
            std::cerr << "Error: URL is required for crawl mode\n";
            printHelp(argv[0]);
            return 1;
        }

        std::cout << "=== CRAWLING MODE ===" << std::endl;
        std::cout << "Starting crawl with options:\n";
        std::cout << "  URL: " << options.url << "\n";
        std::cout << "  Max pages: " << (options.max_pages == -1 ? "unlimited" : std::to_string(options.max_pages)) << "\n";
        std::cout << "  Output dir: " << options.output_dir << "\n";
        std::cout << "  Concurrent threads: " << options.concurrent_threads << "\n";
        
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        CrawlOptions crawl_opts;
        crawl_opts.max_pages = options.max_pages;
        crawl_opts.output_dir = options.output_dir;
        crawl_opts.concurrent_threads = options.concurrent_threads;
        
        WebCrawler crawler(options.url, crawl_opts);
        crawler.crawl();
        
        curl_global_cleanup();

        if (options.processor_mode == "crawl") {
            std::cout << "Crawling completed. Files saved to: " << options.output_dir << std::endl;
            return 0;
        }
    }

    if (options.processor_mode == "process" || options.processor_mode == "both") {
        std::string process_dir = (options.processor_mode == "both") ? options.output_dir : options.input_dir;

        if (process_dir.empty()) {
            std::cerr << "Error: Input directory is required for process mode\n";
            printHelp(argv[0]);
            return 1;
        }

        std::cout << "=== PROCESSING MODE ===" << std::endl;
        std::cout << "Processing files in: " << process_dir << std::endl;
        std::cout << "Processor type: " << options.processor_type << std::endl;
        if (!options.search_query.empty()) {
            std::cout << "Search query: " << options.search_query << std::endl;
        }
        std::cout << "Export format: " << options.export_format << std::endl;
        std::cout << "Export file: " << options.export_file << std::endl;

        // Create processing pipeline
        ProcessingPipeline pipeline(process_dir);
        pipeline.addProcessor(options.processor_type);
        pipeline.setOutputFormat(options.export_format);

        // Process files
        std::vector<ProcessedData> processed_data;
        if (!options.search_query.empty()) {
            auto query = std::make_unique<TextSearchQuery>(options.search_query);
            processed_data = pipeline.processWithFilter(std::move(query));
        } else {
            processed_data = pipeline.processAllFiles();
        }

        std::cout << "Processed " << processed_data.size() << " files" << std::endl;

        // Export results
        if (options.export_format == "database") {
            std::string db_path = options.export_file.empty() ? "processed_data.db" : options.export_file;
            if (pipeline.exportToDatabase(processed_data, db_path)) {
                std::cout << "Results exported to database: " << db_path << std::endl;
            } else {
                std::cerr << "Failed to export to database" << std::endl;
                return 1;
            }
        }else if (options.export_format == "json") {
            std::string json_file = options.export_file.empty() ? "processed_output.json" : options.export_file;
            if (pipeline.exportToJson(processed_data, json_file)) {
                std::cout << "Results exported to JSON: " << json_file << std::endl;
            } else {
                std::cerr << "Failed to export to JSON" << std::endl;
                return 1;
            }
        } else if (options.export_format == "csv") {
            std::string csv_file = options.export_file.empty() ? "processed_output.csv" : options.export_file;
            if (pipeline.exportToCsv(processed_data, csv_file)) {
                std::cout << "Results exported to CSV: " << csv_file << std::endl;
            } else {
                std::cerr << "Failed to export to CSV" << std::endl;
                return 1;
            }
        }
    }

    return 0;
}