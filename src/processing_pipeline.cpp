#include "processing_pipeline.h"
#include "builtin_processors.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp> // You might need to add this dependency

ProcessingPipeline::ProcessingPipeline(const std::string& input_dir) 
    : input_directory(input_dir), output_format("json") {
    // Register built-in processors
    registry.registerProcessor("generic", std::make_unique<GenericProcessor>());
    registry.registerProcessor("text", std::make_unique<TextProcessor>());
    registry.registerProcessor("metadata", std::make_unique<MetadataProcessor>());
    registry.registerProcessor("links", std::make_unique<LinkProcessor>());
}

void ProcessingPipeline::addProcessor(const std::string& processor_name) {
    processor_chain.push_back(processor_name);
}

void ProcessingPipeline::setOutputFormat(const std::string& format) {
    output_format = format;
}

std::vector<ProcessedData> ProcessingPipeline::processAllFiles() {
    std::vector<ProcessedData> results;
    
    if (!std::filesystem::exists(input_directory)) {
        std::cerr << "Input directory does not exist: " << input_directory << std::endl;
        return results;
    }
    
    // Process each HTML file in the directory
    for (const auto& entry : std::filesystem::directory_iterator(input_directory)) {
        if (entry.path().extension() == ".html") {
            std::ifstream file(entry.path(), std::ios::binary);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();
                
                // Use the first processor in the chain (or generic if none specified)
                std::string processor_name = processor_chain.empty() ? "generic" : processor_chain[0];
                ContentProcessor* processor = registry.getProcessor(processor_name);
                
                if (processor) {
                    // Extract URL from filename (this is a simplification)
                    std::string url = "file://" + entry.path().string();
                    ProcessedData data = processor->process(url, content);
                    results.push_back(data);
                } else {
                    std::cerr << "Processor not found: " << processor_name << std::endl;
                }
            }
        }
    }
    
    return results;
}

std::vector<ProcessedData> ProcessingPipeline::processWithFilter(std::unique_ptr<DataQuery> query) {
    std::vector<ProcessedData> all_data = processAllFiles();
    std::vector<ProcessedData> filtered_data;
    
    for (const auto& data : all_data) {
        if (query->matches(data)) {
            filtered_data.push_back(data);
        }
    }
    
    return filtered_data;
}

// Helper function to convert vector of strings to JSON array
nlohmann::json vectorToJson(const std::vector<std::string>& vec) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& item : vec) {
        j.push_back(item);
    }
    return j;
}

// Helper function to convert metadata map to JSON object
nlohmann::json metadataToJson(const std::unordered_map<std::string, std::string>& metadata) {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& pair : metadata) {
        j[pair.first] = pair.second;
    }
    return j;
}

bool ProcessingPipeline::exportToDatabase(const std::vector<ProcessedData>& data, const std::string& db_path) {
    // For now, we'll print a message as database implementation would require more setup
    // In a real implementation, you would connect to SQLite and insert records
    std::cout << "Database export is not fully implemented yet." << std::endl;
    std::cout << "Would export " << data.size() << " records to database: " << db_path << std::endl;
    
    // TODO: Implement actual database export using SQLite
    // This would involve:
    // 1. Opening/creating the SQLite database
    // 2. Creating tables if they don't exist
    // 3. Inserting each ProcessedData record
    
    return true; // Return false if there was an actual error
}

bool ProcessingPipeline::exportToJson(const std::vector<ProcessedData>& data, const std::string& filename) {
    try {
        nlohmann::json j_data = nlohmann::json::array();
        
        for (const auto& item : data) {
            nlohmann::json j_item = {
                {"url", item.url},
                {"title", item.title},
                {"text_content", item.text_content},
                {"html_content", item.html_content},
                {"keywords", vectorToJson(item.keywords)},
                {"links", vectorToJson(item.links)},
                {"images", vectorToJson(item.images)},
                {"metadata", metadataToJson(item.metadata)}
                // Note: We're not serializing the timepoint for simplicity
            };
            j_data.push_back(j_item);
        }
        
        std::ofstream file(filename);
        if (file.is_open()) {
            file << j_data.dump(2); // Dump with indentation for readability
            file.close();
            std::cout << "Successfully exported " << data.size() << " records to JSON: " << filename << std::endl;
            return true;
        } else {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to JSON: " << e.what() << std::endl;
        return false;
    }
}

bool ProcessingPipeline::exportToCsv(const std::vector<ProcessedData>& data, const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        
        // Write CSV header
        file << "URL,Title,Text Content,HTML Content,Keywords,Links,Images\n";
        
        // Write data rows
        for (const auto& item : data) {
            // Simple CSV escaping - replace quotes with double quotes and wrap in quotes
            auto escape_csv = [](const std::string& str) -> std::string {
                std::string result = str;
                // Replace all " with ""
                size_t pos = 0;
                while ((pos = result.find("\"", pos)) != std::string::npos) {
                    result.replace(pos, 1, "\"\"");
                    pos += 2;
                }
                return "\"" + result + "\"";
            };
            
            file << escape_csv(item.url) << ","
                 << escape_csv(item.title) << ","
                 << escape_csv(item.text_content.substr(0, 1000)) << "," // Limit content length
                 << escape_csv(item.html_content.substr(0, 1000)) << "," // Limit content length
                 << escape_csv("") << "," // Keywords (vector)
                 << escape_csv("") << "," // Links (vector)
                 << escape_csv("") << "\n"; // Images (vector)
        }
        
        file.close();
        std::cout << "Successfully exported " << data.size() << " records to CSV: " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to CSV: " << e.what() << std::endl;
        return false;
    }
}