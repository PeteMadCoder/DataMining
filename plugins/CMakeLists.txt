# plugins/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# --- Find the DataMiner Core Library ---
# Since the core library is built in the same project, we don't need find_library.
# CMake targets created by add_library() are directly usable.
# We just need to make sure the target 'dataminer_core' is defined.
# It should be, as main CMakeLists.txt adds src/libdataminer_core before this.

# Check if the target exists (good practice)
if(NOT TARGET dataminer_core)
    message(FATAL_ERROR "DataMiner core library target 'dataminer_core' not found! Ensure src/libdataminer_core is added before plugins.")
endif()

# --- Find DataMiner Core Include Directory ---
# The target 'dataminer_core' should handle include directories via target_include_directories.
# We can get the include dirs if needed, but usually linking the target is sufficient.
# If you need the include dir explicitly for some reason:
get_target_property(DATAMINER_CORE_INCLUDE_DIR dataminer_core INTERFACE_INCLUDE_DIRECTORIES)
# message(STATUS "Found core include dirs: ${DATAMINER_CORE_INCLUDE_DIR}") # For debugging

# --- Check if found (implicitly via target) ---
# If we get here and the target exists, we're good.

# --- Create Plugin Library ---
# Use MODULE for plugins loaded via dlopen (not linked at build time)
add_library(wikipedia_plugin MODULE
    wikipedia_plugin.cpp
    # DO NOT include processor.cpp or plugin_processor.cpp here,
    # they are compiled into the dataminer_core library.
)

# --- Configure Plugin ---
set_target_properties(wikipedia_plugin PROPERTIES
    PREFIX "" # No 'lib' prefix, just 'wikipedia_plugin.so'
    OUTPUT_NAME "wikipedia_plugin"
)

# --- Include Directories for Plugin ---
# The plugin needs to find headers.
target_include_directories(wikipedia_plugin PRIVATE
    # Main project include directory (for plugin_interface.h if you have it there,
    # or if your plugin needs other non-core headers)
    ${CMAKE_SOURCE_DIR}/include
    # Headers from the core library should be available via linking the target
    # ${DATAMINER_CORE_INCLUDE_DIR} # Usually not needed if linking target correctly
    # JSON includes if plugin uses them directly
    ${CMAKE_SOURCE_DIR}/external/json/single_include
)

# --- Link Plugin Against Core Library ---
# This is the crucial part: link the plugin to the core library.
# This makes PluginProcessor, ProcessedData, etc. symbols available to the plugin.
target_link_libraries(wikipedia_plugin
    # PRIVATE linking is appropriate here, the plugin doesn't export these symbols
    PRIVATE dataminer_core
    # Add other libs if the plugin itself needs them (be cautious)
    # Usually, the plugin just needs the core lib symbols.
)

# --- Install Plugin (Optional) ---
install(TARGETS wikipedia_plugin
    LIBRARY DESTINATION plugins # Install to 'plugins' subdirectory
)
