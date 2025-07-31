cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position independent code for dynamic loading and shared libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find required libraries
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)

# Find Gumbo (you might need to adjust this based on your installation)
find_path(GUMBO_INCLUDE_DIR gumbo.h)
find_library(GUMBO_LIBRARY gumbo)

# Platform-specific settings for dynamic loading (dl library)
if(WIN32)
    # Windows uses LoadLibrary, no separate libdl usually needed
    set(DL_LIBRARY "") # Or handle Windows specifics if needed
else()
    # Unix-like systems need libdl for dlopen
    find_library(DL_LIBRARY dl)
    if(NOT DL_LIBRARY)
        message(FATAL_ERROR "libdl not found, required for plugin loading on Unix-like systems")
    endif()
endif()

# Include directories
include_directories(include)
include_directories(include/core)
include_directories(include/processing)
include_directories(include/processors)
include_directories(plugins)

include_directories(${CURL_INCLUDE_DIRS})
include_directories(${GUMBO_INCLUDE_DIR})
include_directories(${SQLite3_INCLUDE_DIRS}) 
include_directories(${CMAKE_SOURCE_DIR}/external/json/single_include)

# Source files
set(SOURCES
    src/processing/main.cpp
    src/core/crawler.cpp
    src/core/downloader.cpp
    src/core/parser.cpp
    src/core/utils.cpp
    src/core/thread_pool.cpp
    src/processors/builtin_processors.cpp
    src/processing/processing_pipeline.cpp
    src/processing/query_system.cpp
    src/processing/plugin_loader.cpp
)

# --- Add Subdirectories ---
add_subdirectory(src/libdataminer_core)

# Create executable
add_executable(DataMiner ${SOURCES})

# --- Link libraries ---
target_link_libraries(DataMiner
    dataminer_core
    ${CURL_LIBRARIES}
    ${GUMBO_LIBRARY}
    ${DL_LIBRARY}
    SQLite::SQLite3
)

# Add subdirectory for plugins
add_subdirectory(plugins)