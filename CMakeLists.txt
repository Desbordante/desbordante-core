# ---- CMake Policies ----

cmake_minimum_required(VERSION 3.25...3.31)

# https://github.com/Desbordante/desbordante-core/issues/549
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
    message(FATAL_ERROR "The project does not currently support versions 4.0 or higher.")
endif()

if(POLICY CMP0167)
    # deprecated by definition, but we use old behaviour for find_package(Boost) to load CMake's FindBoost module
    # TODO: port project to "Modern CMake" https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1
    cmake_policy(SET CMP0167 OLD)
endif()
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

if(POLICY CMP0175)
    # Don't allow invalid arguments of `add_custom_command()`
    cmake_policy(SET CMP0175 NEW)
endif()

# ---- Project ----

project(
    Desbordante
    LANGUAGES CXX
    DESCRIPTION "Open-source data profiling tool"
)

# ---- Build Environment Validation ----

# In-source build validation
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# Untested compiler notification
if(NOT
   ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
)
    message(WARNING "This project has primarily been tested with GCC, LLVM Clang, and Apple Clang."
                    "Other compilers may not be fully supported."
    )
endif()

# --- Build Configuration Options ---

option(COMPILE_TESTS "Build tests" ON)
option(COMPILE_BENCHMARKS "Build benchmarks" OFF)
option(UNPACK_DATASETS "Unpack datasets" ON)
option(BUILD_NATIVE "Build for host machine" ON)
option(USE_LTO "Build using interprocedural optimization" OFF)
option(GDB_DEBUG "Include debug information for use by GDB" OFF)
set(SANITIZER
    ""
    CACHE STRING "Build with sanitizer, possible values: ADDRESS, UB"
)

set(PYTHON
    OFF
    CACHE STRING "Compile Python bindings"
)
set_property(CACHE PYTHON PROPERTY STRINGS OFF COMPILE INSTALL)

# By default select Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(SANITIZER)
    if(SANITIZER STREQUAL "ADDRESS")
        set(ASAN ON)
    elseif(SANITIZER STREQUAL "UB")
        set(UBSAN ON)
    else()
        message(FATAL_ERROR "Unknown sanitizer '${SANITIZER}', try cmake -LH")
    endif()
endif()

if(ASAN AND PYTHON)
    message(
        WARNING
            "ASan runtime must be linked with Python or manually preloaded with LD_PRELOAD=/usr/lib/libasan.so"
    )
endif()

# --- Setting CMake build variables ---

# Enable colored Ninja and compilers diagnostics
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target")

# -DELPP_THREAD_SAFE -- for easylogging++ thread safety
set(BUILD_OPTS "-DELPP_THREAD_SAFE")
if(BUILD_NATIVE)
    string(JOIN ";" BUILD_OPTS "${BUILD_OPTS}" "-march=native")
endif()

# RELEASE build options
string(JOIN ";" RELEASE_BUILD_OPTS "${BUILD_OPTS}" "-O3")

set(DEBUG_BUILD_OPTS "${BUILD_OPTS}")

# Set common DEBUG build options
string(
    JOIN
    ";"
    DEBUG_BUILD_OPTS
    "${DEBUG_BUILD_OPTS}"
    "-g"
    "-Wall"
    "-Wextra"
    "-Werror"
    "-fno-omit-frame-pointer"
    "-fno-optimize-sibling-calls"
)

# Workaround for Clang bug in versions 18 and above:
# See issue https://github.com/llvm/llvm-project/issues/76515
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "18")
    message(WARNING "C++ compiler is Clang++-${CMAKE_CXX_COMPILER_VERSION}. "
                    "Suppressing deprecated declaration warnings. "
                    "Consider using another version of Clang."
    )
    list(APPEND DEBUG_BUILD_OPTS "-Wno-deprecated-declarations")
endif()

# Workaround for googletest bug with char conversions, being recognized by Clang 21+
# See https://github.com/google/googletest/issues/4762
# TODO(senichenkov): remove when googletest gets updated
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "21")
	message(WARNING "Googletest has a bug recognized by Clang 21+. "
					"Supressing character-conversion warning. "
					"Consider using an older version of Clang.")
				list(APPEND DEBUG_BUILD_OPTS "-Wno-error=character-conversion")
endif()

if(ASAN)
    # Set DEBUG build options specific for build with ASAN
    set(ASAN_OPTS "-fsanitize=address")

    execute_process(COMMAND grep -q "Ubuntu" /etc/os-release RESULT_VARIABLE IS_UBUNTU)
    if(IS_UBUNTU EQUAL 0 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # alloc-dealloc-mismatch generates false positives on boost exceptions
        # This applies only to Ubuntu package:
        # https://github.com/llvm/llvm-project/issues/59432?ysclid=m4y0iqca2c577414782
        # Disable this check on files listed in address_sanitizer_ignore_list.txt if compiler
        # is Clang and host distro is Ubuntu:
        message(
            WARNING
                "Running on Ubuntu. ASAN is broken in Ubuntu package, therefore alloc-dealloc-mismatch check will be supressed.
        Consider using another distro for full ASAN coverage"
        )
        string(JOIN ";" ASAN_OPTS
               "-fsanitize-ignorelist=${CMAKE_SOURCE_DIR}/address_sanitizer_ignore_list.txt"
        )
    endif()

    string(
        JOIN
        ";"
        DEBUG_BUILD_OPTS
        "${DEBUG_BUILD_OPTS}"
        "-O1"
        "-Wno-error" # Use of -Werror is discouraged with sanitizers
        # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
        "${ASAN_OPTS}"
    )
    set(DEBUG_LINK_OPTS "${ASAN_OPTS}")
elseif(UBSAN)
    # Set DEBUG build options specific for build with UBSAN
    string(
        JOIN
        ";"
        UBSAN_OPTS
        "-fsanitize=undefined"
        "-fsanitize=float-divide-by-zero"
        "-Wno-error" # Use of -Werror is discouraged with sanitizers
        # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
        "-fno-sanitize=signed-integer-overflow" # Remove this when CustomRandom gets fixed
        "-fno-sanitize=shift" # Remove this when CustomRandom gets fixed
        "-fno-sanitize-recover=all"
    ) # For tests to fail if UBSan finds an error
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_HOST_APPLE)
        # Limit some UB sanitizer checks to "src" directory on macOS when building with Clang,
        # because libraries (STL, googletest, boost, etc.) are somehow broken
        string(JOIN ";" UBSAN_OPTS
               "-fsanitize-ignorelist=${CMAKE_SOURCE_DIR}/ub_sanitizer_ignore_list.txt"
        )
    endif()
    string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}" "-O1" "${UBSAN_OPTS}")
    set(DEBUG_LINK_OPTS "${UBSAN_OPTS}")
else()
    # No sanitizer, just debug build
    string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}" "-O0")
endif()

if(GDB_DEBUG)
    add_compile_options(-ggdb3)
endif()

add_compile_options("$<$<CONFIG:Debug>:${DEBUG_BUILD_OPTS}>")
add_link_options("$<$<CONFIG:Debug>:${DEBUG_LINK_OPTS}>")

add_compile_options("$<$<CONFIG:Release>:${RELEASE_BUILD_OPTS}>")

if(USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(PYTHON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# --- Dependencies installation ---

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
    NAME easyloggingpp
    GITHUB_REPOSITORY abumq/easyloggingpp
    VERSION 9.96.7
    OPTIONS "build_static_lib ON"
    SYSTEM YES
    EXCLUDE_FROM_ALL YES
)
include_directories(SYSTEM "${easyloggingpp_SOURCE_DIR}/src")
set_target_properties(easyloggingpp PROPERTIES COMPILE_FLAGS "-w")

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

if(COMPILE_TESTS)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        VERSION 1.14.0
        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
    )
endif()

if(PYTHON)
    CPMAddPackage("gh:pybind/pybind11@2.13.4")
endif()

# Configure boost
set(Boost_USE_STATIC_LIBS OFF)

set(boost_libraries container thread graph)
if (COMPILE_BENCHMARKS)
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

# --- Core Project Structure and Module Setup ---

include_directories(
    "src/core"
    "src/core/algorithms"
    "src/core/model"
    "src/core/model/types"
    "src/core/parser"
    "src/core/util"
    "src/core/config"
)

add_subdirectory("src/core")

# Configure logs
add_subdirectory("cfg")

# --- Optional Modules Setup ---

# Configure tests
if(COMPILE_TESTS)
    enable_testing()
endif()

if (COMPILE_TESTS OR COMPILE_BENCHMARKS)
    add_subdirectory("src/tests")
endif()

# Configure datasets
if(UNPACK_DATASETS)
    add_subdirectory("datasets")
endif()

# Configure Python bindings
if(PYTHON)
    add_subdirectory("src/python_bindings")
endif()
