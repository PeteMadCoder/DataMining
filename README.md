# DataMining

A modular web crawling and data processing tool designed for extensibility. DataMiner allows you to crawl websites, save the raw HTML, and then process that data using customizable plugins to extract structured information.

---

## Project Organization

The repository is organized as follows:

1.  **`build/`**: The directory where you configure and compile the project using CMake.
2.  **`configs/`**: (Currently empty) Intended for future configuration files.
3.  **`examples/`**: Example files or scripts demonstrating usage.
4.  **`external/`**: External dependencies included in the project (e.g., the nlohmann/json library).
5.  **`include/`**: Header files defining the core interfaces and classes.
    *   `include/core/`: Interfaces for the crawling components (`crawler.h`, `downloader.h`, etc.).
    *   `include/processing/`: Interfaces for the data processing framework (`processor.h`, `processing_pipeline.h`, etc.).
    *   `include/processors/`: Interfaces for built-in processors (`builtin_processors.h`).
6.  **`src/`**: Source code implementing the core logic and the main application.
    *   `src/core/`: Implementation of the crawling components.
    *   `src/libdataminer_core/`: Implementation of the shared core processing library.
    *   `src/processing/`: Implementation of the processing pipeline and the main application entry point (`main.cpp`).
    *   `src/processors/`: Implementation of built-in processors.
7.  **`plugins/`**: Directory for user-created processing plugins. This is where you add custom logic for extracting data.
8.  **`CMakeLists.txt`**: The main CMake build configuration file.
9.  **`scraper.cpp`**: Older/alternative main file; the primary entry is `src/processing/main.cpp`. Very simple, it infinitly searches for through the Web. Very outdated implementation, but nice to see as it was the first implementation.

---

## Compilation

To compile the project, follow these steps:

1.  Navigate to the `build` directory:
    ```bash
    cd build
    ```
2.  Configure the project using CMake:
    ```bash
    cmake ..
    ```
3.  Build the project:
    ```bash
    cmake --build .
    ```
    This will generate the main executable, `DataMiner`, and any plugin shared libraries (e.g., `wikipedia_plugin.so`).

4.  To clean the build directory (remove auto-generated files):
    ```bash
    # While in the build directory
    rm -rf *
    # Or remove specific targets if you prefer
    ```

---

## Usage

DataMiner operates in two main modes: **Crawling** and **Processing**. You can perform both sequentially using the `--both` option.

### Crawling

Crawl a website and save HTML files to a specified output directory.

```bash
# Basic crawl
./DataMiner --url https://example.com

# Crawl with limits
./DataMiner --url https://example.com --max-pages 100 --concurrent-threads 10 --output ./my_crawled_data
```

**Options:**
*   `-u, --url URL`: The starting URL for the crawler.
*   `-m, --max-pages N`: Maximum number of pages to crawl (default: unlimited).
*   `-t, --concurrent-threads N`: Number of threads for concurrent downloads (default: 5).
*   `-o, --output DIR`: Directory to save crawled HTML files (default: `output`).

### Processing

Process previously crawled HTML files using a specific plugin to extract structured data. You can filter which files are processed using the query system.

```bash
# Process files using the 'wikipedia' plugin and export to JSON
./DataMiner --process ./output --processor-type wikipedia --export json --export-file wikipedia_data.json

# Process files using a custom plugin and export to CSV
./DataMiner --process ./my_crawled_data --processor-type my_custom_plugin --export csv --export-file results.csv
```

**Options:**
*   `-p, --process DIR`: Directory containing HTML files to process.
*   `--processor-type TYPE`: Name of the processor/plugin to use (e.g., `generic`, `wikipedia`, or a custom one).
*   `-e, --export FORMAT`: Export format (`json`, `csv`, `database`).
*   `--export-file FILE`: Name of the output file for exported data.
*   `-pt, --processing-threads N`: Number of threads for concurrent processing (default: 4).

*   **Filtering Options:**
    *   `--filter-text TERM`: Filter files containing `TERM` in title or text content.
    *   `--filter-case-sensitive`: Make `--filter-text` case-sensitive.
    *   `--filter-regex PATTERN`: Filter files matching regex `PATTERN` in title or text content.
    *   `--filter-meta-key KEY --filter-meta-value VALUE`: Filter files where metadata has the specified key-value pair.
    *   *(Note: Only one filter type can be applied at a time.)*


### Combined Mode

Crawl a site and then immediately process the results.

```bash
# Crawl then process
./DataMiner --both https://en.wikipedia.org/wiki/Circular_economy --max-pages 50 --processor-type wikipedia --export json --export-file ce_wikipedia_data.json
```

**Option:**
*   `-b, --both URL`: Perform both crawling (starting from URL) and processing.

---

## Plugins

DataMiner's power lies in its plugin system. Plugins define how data is extracted from raw HTML.

*   **Location:** Custom plugins are placed in the `plugins/` directory.
*   **Development:** See `plugins/README.md` for instructions on creating your own plugins.
*   **Built-in Plugin:** An example `wikipedia_plugin.cpp` is included to demonstrate data extraction from Wikipedia pages.

---

## Query System (Filtering)

DataMiner includes a flexible query system for filtering content during the processing stage. This allows you to process only the crawled data that matches specific criteria, saving time and resources.

**Available Query Types:**
*   **`TextSearchQuery`**: Matches pages where the title or main text content contains a specific term. Supports case-sensitive and case-insensitive searches.
*   **`RegexQuery`**: Matches pages where the title or main text content matches a given regular expression.
*   **`MetadataQuery`**: Matches pages that have a specific key-value pair in their extracted metadata.
*   **`AndQuery`**: Combines multiple queries; a page matches only if *all* sub-queries match. *(Defined programmatically)*
*   **`OrQuery`**: Combines multiple queries; a page matches if *any* sub-query matches. *(Defined programmatically)*
*   **`NotQuery`**: Negates another query; a page matches if the sub-query does *not* match. *(Defined programmatically)*

Filtering options are available via the command line as shown in the Usage section.

---

## CPU Thread Recommendation 

To help optimize performance, especially for the processing stage, a helper script `recommend_threads.sh` is available. Running this script will analyze your CPU's core and thread count and suggest an appropriate number of threads to use with the `--processing-threads` option for optimal performance. This is a conservative baseline, however, meaning that it's possible to be more eficient with more threads than those recommended. As such, you should test for your system.

---

## TODOs / Future Improvements

*   Add more built-in processor types.
*   Implement query system for filtering content during processing.
*   Enhance plugin loading to support hot-swapping or configuration-based loading.
*   Improve command-line argument parsing and help messages.
