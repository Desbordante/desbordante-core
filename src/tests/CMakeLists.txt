set(UNIT_TEST_BINARY ${CMAKE_PROJECT_NAME}_test)
set(BENCHMARK_BINARY ${CMAKE_PROJECT_NAME}_benchmark)

file(GLOB common_sources "common/*.h" "common/*.cpp")
file(GLOB unit_test_sources "unit/*.h" "unit/*.cpp")
file(GLOB benchmark_sources "benchmark/*.h" "benchmark/*.cpp")

if (COMPILE_TESTS)
    enable_testing()
    include(GoogleTest)

    # building tests
    add_executable(${UNIT_TEST_BINARY} ${common_sources} ${unit_test_sources})

    # include "tests/common" directory for header inclusion
    target_include_directories(${UNIT_TEST_BINARY} PRIVATE "common")

    # linking with gtest and implemented classes
    target_link_libraries(${UNIT_TEST_BINARY} PRIVATE ${CMAKE_PROJECT_NAME} gtest gmock Boost::graph)

    gtest_discover_tests(${UNIT_TEST_BINARY} WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

if (COMPILE_BENCHMARKS)
	add_executable(${BENCHMARK_BINARY} ${common_sources} ${benchmark_sources})

    # include "tests/common" directory for header inclusion
    target_include_directories(${BENCHMARK_BINARY} PRIVATE "common")

    # linking with implemented classes
    target_link_libraries(${BENCHMARK_BINARY} PRIVATE ${CMAKE_PROJECT_NAME})
endif()

add_custom_target(copy-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test_input_data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data
        )
