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

include_directories(SYSTEM "${spdlog_SOURCE_DIR}/include")

CPMAddPackage("gh:ktprime/emhash#5f327039a2776f38910e600889acf924a2d99ea3")
include_directories(SYSTEM "${emhash_SOURCE_DIR}")

CPMAddPackage(
    NAME frozen
    GITHUB_REPOSITORY serge-sans-paille/frozen
    GIT_TAG 1.2.0
    DOWNLOAD_ONLY True
)
include_directories(SYSTEM "${frozen_SOURCE_DIR}/include")

CPMAddPackage(
    NAME better-enums
    GITHUB_REPOSITORY aantron/better-enums
    GIT_TAG 0.11.3
    DOWNLOAD_ONLY True
)
include_directories(SYSTEM "${better-enums_SOURCE_DIR}")

CPMAddPackage(
    NAME atomicbitvector
    GITHUB_REPOSITORY ekg/atomicbitvector
    GIT_TAG e295358fea9532fa4c37197630d037a4a53ddede
    DOWNLOAD_ONLY True
)
include_directories(SYSTEM "${atomicbitvector_SOURCE_DIR}/include")

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
                "Supressing character-conversion warning. "
                "Consider using an older version of Clang.")
        target_compile_options(gtest PRIVATE "-Wno-error=character-conversion")
    endif()
endif()

if(DESBORDANTE_BINDINGS)
    CPMAddPackage("gh:pybind/pybind11@2.13.4")
endif()

# Configure boost
set(Boost_USE_STATIC_LIBS OFF)

set(boost_libraries container thread graph)
if(DESBORDANTE_BUILD_BENCHMARKS)
    list(APPEND boost_libraries program_options)
endif()

find_package(Boost 1.85.0 REQUIRED COMPONENTS ${boost_libraries})
if(Boost_FOUND)
    message(STATUS "Found Boost include dirs: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Found Boost library dirs: ${Boost_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "Boost not found. If boost is installed specify environment"
                        "variables like \"BOOST_ROOT\" for hint to CMake where to search"
    )
endif()
