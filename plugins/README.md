
# DataMiner Plugins

This directory is where you place custom processing plugins for the DataMiner application. Plugins allow you to define specific ways to extract and process data from the crawled HTML files.

## How Plugins Work

1.  **Plugin Structure:** Each plugin is a separate C++ source file (e.g., `my_custom_plugin.cpp`) that defines one or more data extraction functions and registers them with the DataMiner core.
2.  **Compilation:** When you build the DataMiner project, the build system (`CMakeLists.txt` in this directory) automatically finds the `.cpp` files in this folder and compiles them into separate shared library files (e.g., `my_custom_plugin.so` on Linux).
3.  **Loading:** When DataMiner runs in processing mode, it automatically scans this `plugins` directory for compiled shared libraries (`.so`, `.dll`) and loads them dynamically.
4.  **Registration:** Each loaded plugin must provide a `registerPlugin` function. DataMiner calls this function, passing its internal `ProcessorRegistry`. The plugin uses this to register its processing logic under a unique name.
5.  **Usage:** You specify which processor to use via the command line when running DataMiner in processing mode (e.g., `./DataMiner --process ./output --processor-type my_custom_name`).

## Creating a New Plugin (Using the Template Script)

The easiest way to create a new plugin is to use the provided template script.

1.  **Run the Script:**
    ```bash
    cd plugins
    ./create_plugin.sh my_new_plugin_name
    ```
    This command creates a new file `my_new_plugin_name.cpp` pre-filled with the standard plugin boilerplate code.

2.  **Modify the Template:**
    *   Open the generated `my_new_plugin_name.cpp` file in your editor.
    *   Implement your custom data extraction logic inside the provided extractor functions (e.g., modify `extractCustomData`) or add new ones.
    *   Update the plugin metadata (name, version, description, author) inside the `registerPlugin` function.
    *   Add any necessary `#include` directives for libraries you plan to use (e.g., `<gumbo.h>` for HTML parsing).

3.  **Build DataMiner:**
    Go to your build directory and re-run the build process. The build system will automatically detect your new plugin source file and compile it.
    ```bash
    cd /path/to/your/DataMiner/build
    cmake ..   # Reconfigure to find the new plugin source
    make       # Build the project, including the new plugin
    ```

4.  **Use Your Plugin:**
    Run DataMiner in processing mode and specify your new processor name:
    ```bash
    ./DataMiner --process /path/to/crawled/files --processor-type my_new_plugin_name --export json --export-file my_output.json
    ```


## Creating a New Plugin (Manual Method)

Follow these steps to create your own data processor:

### 1. Create Your Plugin File

Create a new C++ file in this directory, for example, `my_new_processor.cpp`.

### 2. Include Required Headers

Your plugin file needs to include the necessary headers from the DataMiner core library.

```cpp
// my_new_processor.cpp
#include "processing/plugin_processor.h" // Core PluginProcessor class
#include "processing/processor.h"        // ProcessedData structure
// Include any other standard C++ or external library headers you need
#include <regex>
#include <iostream>
// ... other includes ...
```

*Note: The exact relative paths for the DataMiner includes might depend on how they are defined in the main `CMakeLists.txt`. `processing/plugin_processor.h` and `processing/processor.h` should be the key ones if they are installed or available via the build system's include paths.*

### 3. Define Your Data Extraction Logic

Write one or more functions that take the raw HTML content and a reference to the `ProcessedData` object. Your function should analyze the HTML and populate the relevant fields of the `ProcessedData` struct.

```cpp
// Example extractor function
void extractPageTitle(const std::string& html, ProcessedData& data) {
    // --- Use robust parsing like Gumbo (recommended) ---
    // #include <gumbo.h> // Add this include if using Gumbo
    // GumboOutput* output = gumbo_parse(html.c_str());
    // ... use Gumbo C API to navigate and extract ...
    // if (output) gumbo_destroy_output(&kGumboDefaultOptions, output);

    // --- Or, use regex for simple cases (fragile) ---
    std::regex title_regex(R"(<title>(.*?)</title>)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(html, match, title_regex)) {
        data.title = match[1].str();
        // Consider basic cleanup (HTML unescape, trim) as needed
    }

    // You can extract text, links, images, metadata, etc.
    // data.text_content = ...;
    // data.links.push_back(...);
    // data.images.push_back(...);
    // data.metadata["my_key"] = ...;
}

// You can define multiple extractor functions for different pieces of data
void extractCustomMetadata(const std::string& html, ProcessedData& data) {
    // Example: Extract a specific meta tag
    std::regex meta_regex(R"(<meta name="author" content="(.*?)" />)", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(html, match, meta_regex)) {
        data.metadata["author"] = match[1].str();
    }
}
```

### 4. Register Your Plugin

Your plugin must define an `extern "C" void registerPlugin(ProcessorRegistry& registry)` function. This is the entry point that DataMiner will call to load your plugin.

Inside this function:
1.  Create an instance of `PluginProcessor` with a unique name.
2.  Add your extractor functions to this processor instance using `addExtractor()`.
3.  Register the processor instance with the provided `ProcessorRegistry`.

```cpp
// This is the mandatory registration function
extern "C" void registerPlugin(ProcessorRegistry& registry) {
    // It's good practice to print a message
    std::cout << "Registering My New Processor Plugin..." << std::endl;

    // Create the plugin processor instance with a unique name
    // This name is what you will use with --processor-type
    auto processor = std::make_unique<PluginProcessor>("my_new_processor");

    // Add your custom extractor functions
    processor->addExtractor(extractPageTitle);
    processor->addExtractor(extractCustomMetadata);
    // Add more extractors as needed...

    // Register the processor with the DataMiner core
    registry.registerProcessor("my_new_processor", std::move(processor));
}
```

### 5. (Optional) Provide Plugin Information

You can optionally provide functions to give DataMiner more details about your plugin:

```cpp
// Optional: Provide a human-readable name
extern "C" const char* getPluginName() {
    return "My New Processor Plugin";
}

// Optional: Provide a version string
extern "C" const char* getPluginVersion() {
    return "1.0.0";
}

// Optional: Provide a description
extern "C" const char* getPluginDescription() {
    return "A custom plugin to extract page titles and author metadata.";
}
```

### 6. Build DataMiner

Go to your build directory and re-run the build process. The build system will automatically detect your new `my_new_processor.cpp` file and compile it into a shared library (e.g., `my_new_processor.so`).

```bash
cd /path/to/your/DataMiner/build
cmake ..   # Reconfigure to find the new plugin source
make       # Build the project, including the new plugin
```

### 7. Use Your Plugin

Run DataMiner in processing mode and specify your new processor name:

```bash
# Process crawled files using your custom plugin
./DataMiner --process /path/to/crawled/files --processor-type my_new_processor --export json --export-file my_output.json
```

## Tips for Plugin Development

*   **Use Gumbo:** For reliable HTML parsing, prefer using the Gumbo library (`#include <gumbo.h>`) over regular expressions. It's already available in your project and handles malformed HTML gracefully. See `wikipedia_plugin.cpp` for a comprehensive example.
*   **Populate `ProcessedData`:** Make full use of the `ProcessedData` struct fields (`title`, `text_content`, `links`, `images`, `keywords`, `metadata`) to store the extracted information in a structured way.
*   **Handle Errors Gracefully:** Add checks for potential issues like failed regex matches, null Gumbo nodes, invalid configuration values, or file I/O problems. Prevent your plugin from crashing the entire DataMiner process.
*   **Unique Names:** Ensure the name you pass to `PluginProcessor("name")` and `registry.registerProcessor("name", ...)` is unique and descriptive to avoid conflicts.
*   **Test Incrementally:** Test your plugin on a small set of crawled HTML files first to make sure it behaves as expected before running it on large datasets.
*   **Leverage Meta** Use `PluginMetadata` and the optional `getPlugin*` functions to make your plugin more discoverable and user-friendly. Users can list available plugins with `./DataMiner --list-processors`.

## Example Plugins

*   `wikipedia_plugin.cpp`: A comprehensive example showing how to extract various data points from Wikipedia pages using Gumbo.
