# ---- Test Suite Configuration ----

# Define test binary name with project suffix for clarity
# Format: <project_name>_test (e.g. Desbordante_test)
set(UNIT_TEST_BINARY ${CMAKE_PROJECT_NAME}_test)
set(BENCHMARK_BINARY ${CMAKE_PROJECT_NAME}_benchmark)

# ---- Test Sources Setup ----

# Auto-collect test sources
file(GLOB common_sources "common/*.h" "common/*.cpp")
file(GLOB unit_test_sources "unit/*.h" "unit/*.cpp")
file(GLOB benchmark_sources "benchmark/*.h" "benchmark/*.cpp")

if (DESBORDANTE_BUILD_TESTS)
	enable_testing()
	# Load CMake's built-in GoogleTest module
    include(GoogleTest)

	# Create test executable
    add_executable(${UNIT_TEST_BINARY} ${common_sources} ${unit_test_sources})

	# ---- Test Dependencies ----

	# Link test executable with:
	# PRIVATE:
	# - Core library (implementation under test)
	# - GTest/GMock frameworks
	# - Boost.Graph (test-specific dependencies)
    target_link_libraries(${UNIT_TEST_BINARY} PRIVATE ${CMAKE_PROJECT_NAME} gtest gmock Boost::graph)

	# ---- Test Discovery ----

	# Auto-register tests from executable
	# WORKING_DIRECTORY: Ensure test data paths resolve correctly
    gtest_discover_tests(${UNIT_TEST_BINARY} WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

if (DESBORDANTE_BUILD_BENCHMARKS)
	# Create benchmark executable
	add_executable(${BENCHMARK_BINARY} ${common_sources} ${benchmark_sources})

	# ---- Benchmark Dependencies ----

	# Link benchmark executable with:
	# PRIVATE:
	# - Core library (implementation under benchmark)
    target_link_libraries(${BENCHMARK_BINARY} PRIVATE ${CMAKE_PROJECT_NAME})
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
