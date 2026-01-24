# ---- CMake Policies ----

cmake_minimum_required(VERSION 3.26)

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
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|AppleClang|Clang)$")
    message(WARNING "This project has primarily been tested with GCC, LLVM Clang, and Apple Clang."
                    "Other compilers may not be fully supported."
    )
endif()

# --- Build Configuration Options ---

option(DESBORDANTE_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DESBORDANTE_BUILD_NATIVE "Build for host machine" OFF)
option(DESBORDANTE_BUILD_TESTS "Build tests" OFF)
# This option only takes effect if DESBORDANTE_BUILD_TESTS or DESBORDANTE_BUILD_BENCHMARKS is ON
option(DESBORDANTE_FETCH_DATASETS "Fetch datasets for tests or benchmarks" ON)
option(DESBORDANTE_GDB_SYMBOLS "Include debug information for use by GDB" OFF)
# ---- Vertical Hashing Optimization ----
# Performance/functionality trade-off:
# - Disabled by default for maximum performance
# - Enabled for wide datasets (>32 columns) with potential performance penalty
option(DESBORDANTE_SAFE_VERTICAL_HASHING
        "Enable safe vertical hashing mode. Allows processing wide datasets (>32 columns) \
    at the cost of reduced performance. Recommended for exploratory data analysis." OFF
)
option(DESBORDANTE_SANITIZER "Build with sanitizers (UBSan + ASan)" OFF)
option(DESBORDANTE_UNPACK_DATASETS "Unpack datasets" OFF)
option(DESBORDANTE_USE_LTO "Build using interprocedural optimization" OFF)

set(DESBORDANTE_LOG_LEVEL
    ""
    CACHE STRING "Set log level, possible values: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL"
)

set(DESBORDANTE_BINDINGS
    OFF
    CACHE STRING "Build Python bindings"
)
set_property(CACHE DESBORDANTE_BINDINGS PROPERTY STRINGS OFF BUILD INSTALL)

# By default select Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/target")

if(DESBORDANTE_BINDINGS)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    if(DESBORDANTE_SANITIZER)
        message(
            WARNING
            "ASan runtime must be linked with Python or manually preloaded with LD_PRELOAD=/usr/lib/libasan.so"
        )
    endif()
endif()

if(DESBORDANTE_USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include(cmake/desbordante_flags.cmake)

execute_process(COMMAND grep -q "Ubuntu" /etc/os-release RESULT_VARIABLE IS_UBUNTU)
if(DESBORDANTE_SANITIZER AND IS_UBUNTU EQUAL 0 AND CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    # alloc-dealloc-mismatch generates false positives on boost exceptions
    # This applies only to Ubuntu package:
    # https://github.com/llvm/llvm-project/issues/59432
    message(
            WARNING
            "Running on Ubuntu with libc++. ASAN is broken in Ubuntu package with libc++15-19. If you are using \
                these versions of libc++, set ASAN_OPTIONS=alloc_dealloc_mismatch=0 to suppress alloc-dealloc-mismatch \
                check or consider using another distro or higher libc++ version for full ASAN coverage"
    )
endif()

# --- Dependencies installation ---

include(cmake/desbordante_deps.cmake)

# --- Core Project Structure and Module Setup ---

include_directories("src")

add_subdirectory("src/core")

# --- Optional Modules Setup ---

# Configure tests
if(DESBORDANTE_BUILD_TESTS)
    enable_testing()
endif()

if (DESBORDANTE_BUILD_TESTS OR DESBORDANTE_BUILD_BENCHMARKS)
    # Configure the datasets required for tests and benchmarks (downloaded from desbordante-data)
    add_subdirectory("datasets")
    add_subdirectory("src/tests")
endif()

# Configure Python bindings
if(DESBORDANTE_BINDINGS)
    add_subdirectory("src/python_bindings")
endif()
