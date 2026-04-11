include_guard(GLOBAL)

# Set CPM cache location. By default CPM does not enable cache. Here we set the source cache
# location to a place in the source tree. This is only done if we are the top-level project, i.e.
# this CMakeLists is used directly by `cmake ..` or similar to avoid the situation where an
# unrelated project starts caching its dependencies in Desbordante's source tree.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR
   AND NOT DEFINED CPM_SOURCE_CACHE
   AND NOT DEFINED ENV{CPM_SOURCE_CACHE}
)
    set(CPM_SOURCE_CACHE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpm-cache"
        CACHE PATH "Directory to download CPM dependencies"
    )
    message(STATUS "Set CPM source cache location to ${CPM_SOURCE_CACHE}")
endif()

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    GIT_TAG v1.15.2
    OPTIONS "SPDLOG_BUILD_EXAMPLE OFF" "SPDLOG_BUILD_TESTS OFF" "SPDLOG_INSTALL OFF"
            "SPDLOG_HEADER_ONLY ON"
    SYSTEM YES
)

# Set log level based on user input or build type
target_compile_definitions(
        spdlog_header_only
        INTERFACE
        $<$<BOOL:${DESBORDANTE_LOG_LEVEL}>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_${DESBORDANTE_LOG_LEVEL}>
        $<$<AND:$<NOT:$<BOOL:${DESBORDANTE_LOG_LEVEL}>>,$<CONFIG:Debug>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
        $<$<AND:$<NOT:$<BOOL:${DESBORDANTE_LOG_LEVEL}>>,$<NOT:$<CONFIG:Debug>>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)

CPMAddPackage("gh:ktprime/emhash#5f327039a2776f38910e600889acf924a2d99ea3")
add_library(emhash INTERFACE)
target_include_directories(emhash SYSTEM INTERFACE ${emhash_SOURCE_DIR})

CPMAddPackage(
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    VERSION 0.9.7
    OPTIONS "MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF" "MAGIC_ENUM_OPT_BUILD_TESTS OFF"
            "MAGIC_ENUM_OPT_INSTALL OFF"
    SYSTEM YES
)

if(DESBORDANTE_BUILD_TESTS)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
    )
    # Workaround for googletest bug with char conversions, being recognized by Clang 21+
    # See https://github.com/google/googletest/issues/4762
    # TODO(senichenkov): remove when googletest gets updated
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "21")
        message(WARNING "Googletest has a bug recognized by Clang 21+. "
                "Suppressing character-conversion warning. "
                "Consider using an older version of Clang.")
        target_compile_options(gtest PRIVATE "-Wno-error=character-conversion")
    endif()
endif()

# Configure boost
set(Boost_USE_STATIC_LIBS OFF)

set(boost_libraries container thread graph)
if(DESBORDANTE_BUILD_BENCHMARKS)
    list(APPEND boost_libraries program_options)
endif()

find_package(Boost 1.85.0 REQUIRED COMPONENTS ${boost_libraries})
