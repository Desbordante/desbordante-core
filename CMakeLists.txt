cmake_minimum_required(VERSION 3.10)
project(Desbordante)

option(COPY_PYTHON_EXAMPLES "Copy Python examples" OFF)
option(COMPILE_TESTS "Build tests" ON)
option(UNPACK_DATASETS "Unpack datasets" ON)
set(SANITIZER "" CACHE STRING "Build with sanitizer, possible values: ADDRESS, UB")

set(PYTHON OFF CACHE STRING "Compile Python bindings")
set_property(CACHE PYTHON PROPERTY STRINGS OFF COMPILE INSTALL)

# By default select Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

if (SANITIZER)
    if (SANITIZER STREQUAL "ADDRESS")
        set(ASAN ON)
    elseif (SANITIZER STREQUAL "UB")
        set(UBSAN ON)
    else ()
        message(FATAL_ERROR "Unknown sanitizer '${SANITIZER}', try cmake -LH")
    endif ()
endif ()

if (ASAN AND PYTHON)
    message(WARNING "Disabling ASAN with python bindings")
    set(ASAN OFF)
endif()

# compiler and platform-dependent settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/target")
if (MSVC)
    add_compile_options(/MT /MTd /EHsc)
    add_compile_options("$<$<CONFIG:Release>:/O2>")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_BINARY_DIR}/target")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${CMAKE_BINARY_DIR}/target")
else()
    # -DELPP_THREAD_SAFE -- for easylogging++ thread safety
    set(BUILD_OPTS "-DELPP_THREAD_SAFE")

    # RELEASE build options
    string(JOIN ";" RELEASE_BUILD_OPTS "${BUILD_OPTS}"
        "-O3")

    set(DEBUG_BUILD_OPTS "${BUILD_OPTS}")

    # Set common DEBUG build options
    string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}"
        "-g"
        "-Wall"
        "-Wextra"
        "-Werror"
        "-fno-omit-frame-pointer"
        "-fno-optimize-sibling-calls")

    if (ASAN)
        # Set DEBUG build options specific for build with ASAN
        set(ASAN_OPTS "-fsanitize=address")
        string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}"
            "-O1"
            "${ASAN_OPTS}")
        set(DEBUG_LINK_OPTS "${ASAN_OPTS}")
    elseif (UBSAN)
        # Set DEBUG build options specific for build with UBSAN
        string(JOIN ";" UBSAN_OPTS "-fsanitize=undefined"
            "-fsanitize=float-divide-by-zero"
            "-fno-sanitize=signed-integer-overflow" # Remove this when CustomRandom gets fixed
            "-fno-sanitize=shift" # Remove this when CustomRandom gets fixed
            "-fno-sanitize-recover=all") # For tests to fail if UBSan finds an error
        string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}"
            "-O1"
            "${UBSAN_OPTS}")
        set(DEBUG_LINK_OPTS "${UBSAN_OPTS}")
    else ()
        # No sanitizer, just debug build
        string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}"
            "-O0")
    endif()

    add_compile_options("$<$<CONFIG:Debug>:${DEBUG_BUILD_OPTS}>")
    add_link_options("$<$<CONFIG:Debug>:${DEBUG_LINK_OPTS}>")

    add_compile_options("$<$<CONFIG:Release>:${RELEASE_BUILD_OPTS}>")
endif()

# configuring boost
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.72.0 REQUIRED COMPONENTS container thread)
include_directories(${Boost_INCLUDE_DIRS})
message(${Boost_INCLUDE_DIRS})

# providing subdirectories for header inclusion
include_directories(
        "src/core"
        "src/core/algorithms"
        "src/core/model"
        "src/core/model/types"
        "src/core/parser"
        "src/core/util"
        "src/core/config"
)

include_directories(SYSTEM "lib/easyloggingpp/src" "lib/better-enums/")

# adding submodules
if (COMPILE_TESTS)
    add_subdirectory("lib/googletest")
endif()

set( CMAKE_BUILD_TYPE_COPY "${CMAKE_BUILD_TYPE}" )
set( CMAKE_BUILD_TYPE "Release" )
option(build_static_lib "Build easyloggingpp as a static library" ON)
if (PYTHON STREQUAL INSTALL)
    # Relies on undocumented behaviour. EXCLUDE_FROM_ALL is used to prevent install commands
    # inside the easyloggingpp CMakeLists from executing and subsequently failing with a permission error,
    # making it impossible to install the Python package as a normal user.
    add_subdirectory("lib/easyloggingpp" EXCLUDE_FROM_ALL)
else ()
    add_subdirectory("lib/easyloggingpp")
endif ()
set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_COPY} )

add_subdirectory("src/core")

if (COMPILE_TESTS)
    add_subdirectory("src/tests")
endif()

if (UNPACK_DATASETS)
    add_subdirectory("datasets")
endif()

add_subdirectory("cfg")

if (PYTHON)
    add_subdirectory("lib/pybind11")
    add_subdirectory("src/python_bindings")
endif()

if (COPY_PYTHON_EXAMPLES)
    add_subdirectory("examples")
endif()
