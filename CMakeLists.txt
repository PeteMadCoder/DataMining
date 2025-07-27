cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required libraries
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)

# Find Gumbo (you might need to adjust this based on your installation)
find_path(GUMBO_INCLUDE_DIR gumbo.h)
find_library(GUMBO_LIBRARY gumbo)

# Include directories
include_directories(include)
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${GUMBO_INCLUDE_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/crawler.cpp
    src/downloader.cpp
    src/parser.cpp
    src/utils.cpp
    src/thread_pool.cpp
)

# Create executable
add_executable(webcrawler ${SOURCES})

# Link libraries
target_link_libraries(webcrawler ${CURL_LIBRARIES})
target_link_libraries(webcrawler ${GUMBO_LIBRARY})