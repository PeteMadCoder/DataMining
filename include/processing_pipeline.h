#pragma once
#include "processor.h"
#include "query_system.h"
#include <filesystem>
#include <queue>

class ProcessingPipeline {
private:
    ProcessorRegistry registry;
    std::vector<std::string> processor_chain;
    std::string input_directory;
    std::string output_format;  // "database", "json", "csv"
    
public:
    ProcessingPipeline(const std::string& input_dir);
    
    void addProcessor(const std::string& processor_name);
    void setOutputFormat(const std::string& format);
    
    // Process all files in directory
    std::vector<ProcessedData> processAllFiles();
    
    // Process with filtering
    std::vector<ProcessedData> processWithFilter(std::unique_ptr<DataQuery> query);
    
    // Export results
    bool exportToDatabase(const std::vector<ProcessedData>& data, const std::string& db_path);
    bool exportToJson(const std::vector<ProcessedData>& data, const std::string& filename);
    bool exportToCsv(const std::vector<ProcessedData>& data, const std::string& filename);

};