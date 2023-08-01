set(BINARY ${CMAKE_PROJECT_NAME}_test)

# getting GTest
enable_testing()

# building tests
file(GLOB_RECURSE test_sources "*.h*" "*.cpp*")
add_executable(${BINARY} ${test_sources})
add_test(NAME ${BINARY} COMMAND ${BINARY})

# linking with gtest and implemented classes
target_link_libraries(${BINARY} PRIVATE ${CMAKE_PROJECT_NAME}_lib gtest gmock)

# copying sample csv's for testing
add_custom_target(copy-files ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/tests/input_data
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/input_data
        )
