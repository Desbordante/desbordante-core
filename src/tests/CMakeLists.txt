set(BINARY ${CMAKE_PROJECT_NAME}_test)

# getting GTest
include(GoogleTest)

# building tests
file(GLOB_RECURSE test_sources "*.h*" "*.cpp*")
add_executable(${BINARY} ${test_sources})

# linking with gtest and implemented classes
target_link_libraries(${BINARY} PRIVATE ${CMAKE_PROJECT_NAME} gtest gmock Boost::graph)

gtest_discover_tests(${BINARY} WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# copying sample csv's for testing
add_custom_target(copy-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/test_input_data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data
        )
