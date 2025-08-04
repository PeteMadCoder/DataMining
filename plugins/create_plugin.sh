#!/bin/bash

# --- create_plugin.sh ---
# A simple script to generate a new DataMiner plugin template.

# Check if a plugin name was provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <plugin_name>"
    echo "Example: $0 my_custom_extractor"
    exit 1
fi

PLUGIN_NAME="$1"
PLUGIN_FILE="${PLUGIN_NAME}.cpp"

# Check if the file already exists to prevent overwriting
if [ -f "$PLUGIN_FILE" ]; then
    echo "Error: File '$PLUGIN_FILE' already exists!"
    exit 1
fi

# Get current date for copyright/author placeholder (optional)
CURRENT_YEAR=$(date +"%Y")

# --- Generate the plugin template ---
cat > "$PLUGIN_FILE" <<EOF
/*
 * DataMiner Plugin Template
 *
 * Plugin Name: $PLUGIN_NAME
 * Description: A custom DataMiner plugin.
 * Author: Your Name
 * Version: 1.0.0
 * Date: $(date)
 */

#include "processing/plugin_processor.h"
#include "processing/processor.h"
// Include Gumbo for robust HTML parsing
// #include <gumbo.h>
#include <regex>
#include <iostream>
// Include any other standard C++ libraries you need
// #include <string>
// #include <vector>
// ...

// --- Utility Functions (Optional) ---
// Add any helper functions for string manipulation, data cleaning, etc.
// std::string trim(const std::string& str) { ... }
// std::string unescapeHtml(const std::string& text) { ... }


// --- Extractor Functions ---
// Define your data extraction logic here.
// Each function should take the HTML string and a reference to ProcessedData.

/**
 * @brief Extracts the page title from the HTML.
 * @param html The raw HTML content of the page.
 * @param data The ProcessedData struct to populate.
 */
void extractTitle(const std::string& html, ProcessedData& data) {
    // Example using regex (fragile, for simple cases)
    std::regex title_regex(R"(<title>(.*?)</title>)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(html, match, title_regex)) {
        data.title = match[1].str();
        // Consider adding basic cleanup (HTML unescape, trim) as needed
        // data.title = trim(unescapeHtml(data.title));
    }

    // Example using Gumbo (recommended for robust parsing)
    /*
    #include <gumbo.h> // Make sure to include this at the top
    GumboOutput* output = gumbo_parse(html.c_str());
    if (output && output->root) {
        // ... use Gumbo C API to find <title> tag ...
        // See wikipedia_plugin.cpp for a detailed example.
    }
    if (output) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    */
}

/**
 * @brief Example extractor for custom metadata or other content.
 * @param html The raw HTML content of the page.
 * @param data The ProcessedData struct to populate.
 */
void extractCustomData(const std::string& html, ProcessedData& data) {
    // Example: Extract a specific meta tag
    // std::regex meta_regex(R"(<meta name="author" content="(.*?)" />)", std::regex_constants::icase);
    // std::smatch match;
    // if (std::regex_search(html, match, meta_regex)) {
    //     data.metadata["author"] = match[1].str();
    // }

    // Example: Extract all links (href attributes)
    // std::regex link_regex(R"(<a[^>]*href\s*=\s*["'](.*?)["'][^>]*>)", std::regex_constants::icase);
    // std::sregex_iterator iter(html.begin(), html.end(), link_regex);
    // std::sregex_iterator end;
    // for (; iter != end; ++iter) {
    //     std::string url = iter->str(1);
    //     // Basic validation or transformation
    //     if (!url.empty() && url[0] == '/') {
    //         // Convert relative URL to absolute (example, needs base URL logic)
    //         // url = "https://example.com" + url;
    //     }
    //     data.links.push_back(url);
    // }

    // Add your custom extraction logic here...
    // You can populate data.text_content, data.images, data.keywords, data.metadata, etc.
}


// --- Plugin Registration ---
// This is the mandatory function that DataMiner calls to load the plugin.

/**
 * @brief Registers the plugin with the DataMiner core.
 * @param registry The ProcessorRegistry provided by DataMiner.
 */
extern "C" void registerPlugin(ProcessorRegistry& registry) {
    std::cout << "Registering plugin: $PLUGIN_NAME" << std::endl;

    // Define plugin metadata (optional but recommended)
    PluginMetadata meta;
    meta.name = "$PLUGIN_NAME";
    meta.version = "1.0.0";
    meta.description = "A custom DataMiner plugin generated from template.";
    meta.author = "Your Name";

    // Create the plugin processor instance with a unique name
    // This name is what you will use with --processor-type
    auto processor = std::make_unique<PluginProcessor>("$PLUGIN_NAME", meta);

    // Add your custom extractor functions
    processor->addExtractor(extractTitle);
    processor->addExtractor(extractCustomData);
    // Add more extractors as needed...

    // Register the processor with the DataMiner core
    registry.registerProcessor("$PLUGIN_NAME", std::move(processor));
    
    std::cout << "Plugin '$PLUGIN_NAME' registered successfully." << std::endl;
}

// --- Optional Plugin Information Functions ---
// Provide richer details about the plugin.

/**
 * @brief Gets the human-readable name of the plugin.
 * @return The plugin name.
 */
extern "C" const char* getPluginName() {
    return "$PLUGIN_NAME";
}

/**
 * @brief Gets the version of the plugin.
 * @return The version string.
 */
extern "C" const char* getPluginVersion() {
    return "1.0.0";
}

/**
 * @brief Gets the description of the plugin.
 * @return The description string.
 */
extern "C" const char* getPluginDescription() {
    return "A custom DataMiner plugin generated from template. Modify extractCustomData to add your logic.";
}

EOF

# Make the script executable (itself)
chmod +x "$0"

echo "Plugin template '$PLUGIN_FILE' created successfully!"
echo "You can now edit this file to implement your data extraction logic."
echo "After editing, rebuild DataMiner to compile your new plugin."
