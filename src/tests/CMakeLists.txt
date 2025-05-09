set(UNIT_TEST_BINARY ${CMAKE_PROJECT_NAME}_test)
set(PERF_TEST_BINARY ${CMAKE_PROJECT_NAME}_perf_test)

file(GLOB common_sources "common/*.h" "common/*.cpp")
file(GLOB unit_test_sources "unit_tests/*.h" "unit_tests/*.cpp")
file(GLOB perf_test_sources "performance_tests/*.h" "performance_tests/*.cpp")

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

if (COMPILE_PERFORMANCE_TESTS)
    add_executable(${PERF_TEST_BINARY} ${common_sources} ${perf_test_sources})

    # include "tests/common" directory for header inclusion
    target_include_directories(${PERF_TEST_BINARY} PRIVATE "common")

    # linking with implemented classes
    target_link_libraries(${PERF_TEST_BINARY} PRIVATE ${CMAKE_PROJECT_NAME})
endif()

add_custom_target(copy-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test_input_data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data
        )
