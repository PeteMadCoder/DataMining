#include "processing_pipeline.h"
#include "builtin_processors.h"
#include "plugin_processor.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "plugin_interface.h"
#include <sqlite3.h>
#include <sstream>
#include <future>
#include <mutex>
#include <algorithm>

ProcessingPipeline::ProcessingPipeline(const std::string& input_dir, const std::string& plugins_dir, size_t threads) 
    : input_directory(input_dir), output_format("json"), plugins_directory(plugins_dir), num_threads(threads) {
    // Register built-in processors
    registry.registerProcessor("generic", std::make_unique<GenericProcessor>());
    registry.registerProcessor("text", std::make_unique<TextProcessor>());
    registry.registerProcessor("metadata", std::make_unique<MetadataProcessor>());
    registry.registerProcessor("links", std::make_unique<LinkProcessor>());

    // Load plugins automatically
    loadPlugins();

    // Init thread pool
    if (num_threads > 0) {
        thread_pool = std::make_unique<ThreadPool>(num_threads);
        std::cout << "Initialized thead pool with " << num_threads << " threads." << std::endl; 
    } else {
        std::cout << "Processing will run synchronously (0 threads specified)." << std::endl;
    }
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
    
    // Collect all html files first
    std::vector<std::filesystem::directory_entry> html_files;
    for (const auto& entry : std::filesystem::directory_iterator(input_directory)) {
        if (entry.path().extension() == ".html") {
            html_files.push_back(entry);
        }
    }

    if (html_files.empty()) {
        std::cout << "No html files found in directory: " << input_directory << std::endl;
        return results;
    }

    std::cout << "Found " << html_files.size() << " HTML files to process." << std::endl;

    if (thread_pool && num_threads > 0) {
        // --- Concurrent Processing ---
        std::cout << "Processing files concurrently using " << num_threads << " threads..." << std::endl;
        std::vector<std::future<std::unique_ptr<ProcessedData>>> futures;

        // Submit all tasks to the thread pool
        for (const auto& entry : html_files) {
            futures.push_back(
                thread_pool->enqueue([this, entry]() { // Pass 'this' to access member functions
                    return this->processSingleFile(entry);
                })
            );
        }

        // Collect the results as futures complete
        results.reserve(html_files.size());
        for (auto& future : futures) {
            try {
                auto result = future.get(); // This will block until the task is done
                if (result) {
                    results.push_back(std::move(*result));
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception occured during file processing: " << e.what() << std::endl;
            }
        }
    } else {
        // --- Synchronous Processing (Fallback) ---
        std::cout << "Processing files synchronously..." << std::endl;
        results.reserve(html_files.size());

        for (const auto& entry : html_files) {
            auto result = processSingleFile(entry);
            if (result) {
                results.push_back(std::move(*result));
            }
        }
    }
    
    return results;
}

std::vector<ProcessedData> ProcessingPipeline::processWithFilter(std::unique_ptr<DataQuery> query) {
    // Get all processed data first
    std::vector<ProcessedData> all_data = processAllFiles(); // This now uses threads
    
    // Then filter it
    std::vector<ProcessedData> filtered_data;
    filtered_data.reserve(all_data.size()); // Reserve potential space

    for (const auto& data : all_data) {
        if (query->matches(data)) {
            filtered_data.push_back(data);
        }
    }
    
    std::cout << "Filtering complete: " << filtered_data.size() << " out of " << all_data.size() << " files matched the query." << std::endl;
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

bool ProcessingPipeline::loadPlugins() {
    std::cout << "Searching for plugins in: " << plugins_directory << std::endl;

    // Find all plugin files
    std::vector<std::string> plugin_paths = PluginLoader::findPlugins(plugins_directory);

    // Load each plugin
    for (const std::string& plugin_path : plugin_paths) {
        if (!PluginLoader::loadPlugin(plugin_path, registry)) {
            std::cerr << "Failed to load plugin: " << plugin_path << std::endl;
        }
    }

    return true;
}

bool ProcessingPipeline::exportToDatabase(const std::vector<ProcessedData>& data, const std::string& db_path) {
    sqlite3* db;
    int rc;
    char* errMsg = 0;

    // 1. Open or create the database file
    rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Can't open database (" << db_path << "): " << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return false;
    }

    std::cout << "Successfully opened/created database: " << db_path << std::endl;

    // 2. Begin a transaction for performance and atomicity
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << (errMsg ? errMsg : "Unknown error") << std::endl;
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // 3. Create tables with foreign key relationships
    const char* create_schema_sql = R"(
        CREATE TABLE IF NOT EXISTS pages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT UNIQUE NOT NULL,
            title TEXT,
            text_content TEXT,
            html_content TEXT,
            processed_time TEXT -- Store as ISO 8601 string
        );

        CREATE TABLE IF NOT EXISTS keywords (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            keyword TEXT NOT NULL,
            FOREIGN KEY (page_id) REFERENCES pages (id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS links (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            link TEXT NOT NULL,
            FOREIGN KEY (page_id) REFERENCES pages (id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS images (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            image_url TEXT NOT NULL,
            FOREIGN KEY (page_id) REFERENCES pages (id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS metadata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            page_id INTEGER NOT NULL,
            key TEXT NOT NULL,
            value TEXT NOT NULL,
            FOREIGN KEY (page_id) REFERENCES pages (id) ON DELETE CASCADE
        );

        -- Create indexes for faster lookups
        CREATE INDEX IF NOT EXISTS idx_pages_url ON pages(url);
        CREATE INDEX IF NOT EXISTS idx_keywords_page_id ON keywords(page_id);
        CREATE INDEX IF NOT EXISTS idx_links_page_id ON links(page_id);
        CREATE INDEX IF NOT EXISTS idx_images_page_id ON images(page_id);
        CREATE INDEX IF NOT EXISTS idx_metadata_page_id ON metadata(page_id);
        CREATE INDEX IF NOT EXISTS idx_keywords_keyword ON keywords(keyword);
        CREATE INDEX IF NOT EXISTS idx_links_link ON links(link);
    )";

    rc = sqlite3_exec(db, create_schema_sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error creating schema: " << (errMsg ? errMsg : "Unknown error") << std::endl;
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0); // Rollback transaction on error
        sqlite3_close(db);
        return false;
    }

    // 4. Prepare SQL statements for inserting data
    const char* insert_page_sql = R"(
        INSERT OR REPLACE INTO pages (url, title, text_content, html_content, processed_time)
        VALUES (?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* insert_page_stmt;
    rc = sqlite3_prepare_v2(db, insert_page_sql, -1, &insert_page_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare page insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    const char* insert_keyword_sql = "INSERT INTO keywords (page_id, keyword) VALUES (?, ?);";
    sqlite3_stmt* insert_keyword_stmt;
    rc = sqlite3_prepare_v2(db, insert_keyword_sql, -1, &insert_keyword_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare keyword insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(insert_page_stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    const char* insert_link_sql = "INSERT INTO links (page_id, link) VALUES (?, ?);";
    sqlite3_stmt* insert_link_stmt;
    rc = sqlite3_prepare_v2(db, insert_link_sql, -1, &insert_link_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare link insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(insert_page_stmt);
        sqlite3_finalize(insert_keyword_stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    const char* insert_image_sql = "INSERT INTO images (page_id, image_url) VALUES (?, ?);";
    sqlite3_stmt* insert_image_stmt;
    rc = sqlite3_prepare_v2(db, insert_image_sql, -1, &insert_image_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare image insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(insert_page_stmt);
        sqlite3_finalize(insert_keyword_stmt);
        sqlite3_finalize(insert_link_stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    const char* insert_metadata_sql = "INSERT INTO metadata (page_id, key, value) VALUES (?, ?, ?);";
    sqlite3_stmt* insert_metadata_stmt;
    rc = sqlite3_prepare_v2(db, insert_metadata_sql, -1, &insert_metadata_stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare metadata insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(insert_page_stmt);
        sqlite3_finalize(insert_keyword_stmt);
        sqlite3_finalize(insert_link_stmt);
        sqlite3_finalize(insert_image_stmt);
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return false;
    }

    bool success = true;
    int pages_inserted = 0;

    // 5. Insert data
    for (const auto& item : data) {
        sqlite3_int64 page_id = -1;

        // --- Insert into 'pages' table ---
        // Convert time point to ISO 8601 string
        std::time_t tt = std::chrono::system_clock::to_time_t(item.processed_time);
        std::tm* gmt_tm = std::gmtime(&tt); // Use gmtime for UTC
        std::ostringstream time_stream;
        if (gmt_tm) {
             time_stream << std::put_time(gmt_tm, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 UTC
        } else {
             time_stream << "1970-01-01T00:00:00Z"; // Fallback
        }
        std::string time_str = time_stream.str();

        sqlite3_bind_text(insert_page_stmt, 1, item.url.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_page_stmt, 2, item.title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_page_stmt, 3, item.text_content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_page_stmt, 4, item.html_content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_page_stmt, 5, time_str.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(insert_page_stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert page (" << item.url << "): " << sqlite3_errmsg(db) << std::endl;
            success = false;
            // Continue processing other items, but note the error
        } else {
            pages_inserted++;
            // Get the ID of the inserted or replaced page
            page_id = sqlite3_last_insert_rowid(db);
        }

        // Reset page statement for next iteration
        sqlite3_reset(insert_page_stmt);
        sqlite3_clear_bindings(insert_page_stmt);

        // If page insertion failed or we couldn't get the ID, skip related data
        if (page_id == -1) {
            continue;
        }

        // --- Insert into 'keywords' table ---
        for (const auto& keyword : item.keywords) {
            sqlite3_bind_int64(insert_keyword_stmt, 1, page_id);
            sqlite3_bind_text(insert_keyword_stmt, 2, keyword.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(insert_keyword_stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to insert keyword for page (" << item.url << "): " << sqlite3_errmsg(db) << std::endl;
                success = false; // Mark overall failure but continue
            }
            sqlite3_reset(insert_keyword_stmt);
            sqlite3_clear_bindings(insert_keyword_stmt);
        }

        // --- Insert into 'links' table ---
        for (const auto& link : item.links) {
            sqlite3_bind_int64(insert_link_stmt, 1, page_id);
            sqlite3_bind_text(insert_link_stmt, 2, link.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(insert_link_stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to insert link for page (" << item.url << "): " << sqlite3_errmsg(db) << std::endl;
                success = false;
            }
            sqlite3_reset(insert_link_stmt);
            sqlite3_clear_bindings(insert_link_stmt);
        }

        // --- Insert into 'images' table ---
        for (const auto& image : item.images) {
            sqlite3_bind_int64(insert_image_stmt, 1, page_id);
            sqlite3_bind_text(insert_image_stmt, 2, image.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(insert_image_stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to insert image for page (" << item.url << "): " << sqlite3_errmsg(db) << std::endl;
                success = false;
            }
            sqlite3_reset(insert_image_stmt);
            sqlite3_clear_bindings(insert_image_stmt);
        }

        // --- Insert into 'metadata' table ---
        for (const auto& meta_pair : item.metadata) {
            sqlite3_bind_int64(insert_metadata_stmt, 1, page_id);
            sqlite3_bind_text(insert_metadata_stmt, 2, meta_pair.first.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(insert_metadata_stmt, 3, meta_pair.second.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_step(insert_metadata_stmt);
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to insert metadata for page (" << item.url << "): " << sqlite3_errmsg(db) << std::endl;
                success = false;
            }
            sqlite3_reset(insert_metadata_stmt);
            sqlite3_clear_bindings(insert_metadata_stmt);
        }
    }

    // 6. Finalize prepared statements
    sqlite3_finalize(insert_page_stmt);
    sqlite3_finalize(insert_keyword_stmt);
    sqlite3_finalize(insert_link_stmt);
    sqlite3_finalize(insert_image_stmt);
    sqlite3_finalize(insert_metadata_stmt);

    // 7. Commit or rollback transaction
    if (success) {
        rc = sqlite3_exec(db, "COMMIT;", 0, 0, &errMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "Failed to commit transaction: " << (errMsg ? errMsg : "Unknown error") << std::endl;
            if (errMsg) sqlite3_free(errMsg);
            success = false; // Mark as failed if commit fails
        } else {
            std::cout << "Successfully exported " << pages_inserted << " pages (and related data) to database: " << db_path << std::endl;
        }
    } else {
        std::cerr << "Errors occurred during export. Attempting to rollback..." << std::endl;
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0); // Ignore errors on rollback
        std::cerr << "Export to database failed. " << pages_inserted << " pages processed before error." << std::endl;
    }

    // 8. Close database connection
    sqlite3_close(db);

    return success;
}

std::unique_ptr<ProcessedData> ProcessingPipeline::processSingleFile(const std::filesystem::directory_entry& entry) {
    if (entry.path().extension() != ".html") {
        return nullptr; // Skip non html files
    }

    std::ifstream file(entry.path(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for processing: " << entry.path() << std::endl;
        return nullptr;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Use the first processor in the chain (or generic if none specified)
    std::string processor_name = processor_chain.empty() ? "generic" : processor_chain[0];
    ContentProcessor* processor = registry.getProcessor(processor_name);

    if (processor) {
        // Extract URL from filename (this is a simplification)
        std::string url = "file://" + entry.path().string();
        ProcessedData data = processor->process(url, content);
        return std::make_unique<ProcessedData>(std::move(data));
    } else {
        std::cerr << "Processor not found: " << processor_name << std::endl;
        return nullptr;
    }
}