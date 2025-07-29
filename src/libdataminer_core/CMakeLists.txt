# src/libdataminer_core/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Enable PIC for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Define library sources ---
# These paths are relative to THIS directory (src/libdataminer_core/)
# Based on your structure, the files are in the same directory as this CMakeLists.txt
set(LIB_SOURCES
    processor.cpp        # File is in src/libdataminer_core/processor.cpp
    plugin_processor.cpp # File is in src/libdataminer_core/plugin_processor.cpp
    # Add other core processing .cpp files here if they are part of the core lib
    # If you decided ProcessingPipeline belongs here, add it:
    # processing_pipeline.cpp
)

# --- Define library public headers ---
# These are the headers that users of the library (main app, plugins) will need to include.
# They reside in the main project's include/ directory.
set(LIB_HEADERS
    # Point to the headers in the main include directory
    ${CMAKE_SOURCE_DIR}/include/processing/processor.h
    ${CMAKE_SOURCE_DIR}/include/processing/plugin_processor.h
    # Add paths to other core processing .h files from include/processing/ here
    # If ProcessingPipeline.h is moved to include/processing/, add it:
    # ${CMAKE_SOURCE_DIR}/include/processing/processing_pipeline.h
)

# --- Check if sources exist (optional but good practice) ---
foreach(src_file ${LIB_SOURCES})
    # Prepend the current source dir to get the full path for checking
    set(full_path ${CMAKE_CURRENT_SOURCE_DIR}/${src_file})
    if(NOT EXISTS ${full_path})
        message(FATAL_ERROR "Required source file not found: ${full_path}")
    endif()
endforeach()

# --- Create the shared library ---
add_library(dataminer_core SHARED ${LIB_SOURCES})

# --- Set library properties ---
set_target_properties(dataminer_core PROPERTIES
    VERSION "1.0.0"
    SOVERSION 1
    PUBLIC_HEADER "${LIB_HEADERS}"
)

# --- Include directories for the library itself ---
target_include_directories(dataminer_core
    PUBLIC
        # Make the main project's include directory available to library users
        # This allows users (main app, plugins) to do #include "processing/processor.h"
        # $<BUILD_INTERFACE:...> applies when the target is used within the same build.
        # $<INSTALL_INTERFACE:...> applies when the target is found via find_package after installation.
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> # <-- Key Fix: Path for headers during build
        $<INSTALL_INTERFACE:include>                   # <-- Path for headers after installation
    PRIVATE
        # Add any private includes needed only by the lib's own .cpp files during build
        # This is crucial for the library's .cpp files to find headers in include/
        ${CMAKE_SOURCE_DIR}/include                    # <-- Key Fix: Ensure lib's .cpp can find public headers
        # Add paths to private headers if they exist within this lib's source dir
        # ${CMAKE_CURRENT_SOURCE_DIR} # If you have private .h files here
        # ${CMAKE_CURRENT_SOURCE_DIR}/detail # If you have a detail subdirectory for private headers
        # Include paths for external dependencies needed internally by the lib's .cpp files
        ${CMAKE_SOURCE_DIR}/external/json/single_include # If lib core uses JSON internally
)

# --- Link dependencies needed by the library ---
# Link Gumbo PRIVATEly if only the library's internal code uses it.
target_link_libraries(dataminer_core PRIVATE ${GUMBO_LIBRARY})

# --- Install rules (Optional for now) ---
install(TARGETS dataminer_core
    EXPORT DataMinerCoreTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin # For Windows DLLs
    PUBLIC_HEADER DESTINATION include/processing # Install headers to match include path
    INCLUDES DESTINATION include # Add base include dir to interface for installed version
)

# --- Export for find_package within build tree ---
# This allows other parts of the same build (like plugins) to find this target easily.
export(TARGETS dataminer_core
       NAMESPACE DataMiner::
       FILE ${CMAKE_CURRENT_BINARY_DIR}/DataMinerCoreTargets.cmake # Export to build dir
)
