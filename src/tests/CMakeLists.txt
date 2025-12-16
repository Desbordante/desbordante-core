# ---- Test Suite Configuration ----

# ---- Test Sources Setup ----
add_subdirectory(common)

if(DESBORDANTE_BUILD_TESTS)
    add_subdirectory(unit)
endif()
if(DESBORDANTE_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# ---- Test Data Preparation ----

# Copy test datasets to build output directory before any build target
# ALL ensures this runs by default for all build operations
add_custom_target(
    copy-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/test_input_data
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data
    COMMENT "Copying test datasets to build directory"
)

# ---- Build Cleanup Configuration ----

# Register test data directory for automatic removal during cleanup
# Ensures test artifacts are deleted with `cmake --build . --target clean`
set_property(
    DIRECTORY
    APPEND
    PROPERTY ADDITIONAL_CLEAN_FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data"
)
