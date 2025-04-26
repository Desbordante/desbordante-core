# ---- Test Suite Configuration ----

# Define test binary name with project suffix for clarity
# Format: <project_name>_test (e.g. Desbordante_test)
set(BINARY ${CMAKE_PROJECT_NAME}_test)

# Load CMake's built-in GoogleTest module
include(GoogleTest)

# ---- Test Sources Setup ----

# Auto-collect test sources recursively
file(GLOB_RECURSE test_sources "*.h*" "*.cpp*")

# Create test executable
add_executable(${BINARY} ${test_sources})

# ---- Test Dependencies ----

# Link test executable with:
# PRIVATE:
# - Core library (implementation under test)
# - GTest/GMock frameworks
# - Boost.Graph (test-specific dependencies)
target_link_libraries(
    ${BINARY}
    PRIVATE ${CMAKE_PROJECT_NAME} # Core library with algorithms
            gtest # Google Test framework
            gmock # Google Mock framework
            Boost::graph # Graph components used in tests
)

# ---- Test Discovery ----

# Auto-register tests from executable
# WORKING_DIRECTORY: Ensure test data paths resolve correctly
gtest_discover_tests(
    ${BINARY} WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" # ${CMAKE_BINARY_DIR}/target
)

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
