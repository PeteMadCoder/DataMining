#pragma once
#include "processor.h"
#include "query_system.h"
#include "thread_pool.h"
#include <filesystem>
#include <queue>
#include <string>
#include <vector>
#include <sqlite3.h>
#include <memory>

class ProcessingPipeline {
private:
    ProcessorRegistry registry;
    std::vector<std::string> processor_chain;
    std::string input_directory;
    std::string output_format;  // "database", "json", "csv"
    std::string plugins_directory;
    std::unique_ptr<ThreadPool> thread_pool;
    size_t num_threads;
    
public:
    ProcessingPipeline(const std::string& input_dir,
        const std::string& plugins_dir = "plugins",
        size_t threads = 4
    );
    
    void addProcessor(const std::string& processor_name);
    void setOutputFormat(const std::string& format);
    bool loadPlugins();
    
    // Process all files in directory
    std::vector<ProcessedData> processAllFiles();
    
    // Process with filtering
    std::vector<ProcessedData> processWithFilter(std::unique_ptr<DataQuery> query);

    std::unique_ptr<ProcessedData> processSingleFile(const std::filesystem::directory_entry& entry);
    
    // Export results
    bool exportToDatabase(const std::vector<ProcessedData>& data, const std::string& db_path);
    bool exportToJson(const std::vector<ProcessedData>& data, const std::string& filename);
    bool exportToCsv(const std::vector<ProcessedData>& data, const std::string& filename);

    ProcessorRegistry& getRegistry() { return registry; }
};