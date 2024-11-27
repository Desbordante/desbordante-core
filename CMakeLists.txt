cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0144 NEW)
project(Desbordante)

option(COPY_PYTHON_EXAMPLES "Copy Python examples" OFF)
option(COMPILE_TESTS "Build tests" ON)
option(UNPACK_DATASETS "Unpack datasets" ON)
option(BUILD_NATIVE "Build for host machine" ON)
option(USE_LTO "Build using interprocedural optimization" OFF)
option(GDB_DEBUG "Include debug information for use by GDB" OFF)
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
    message(WARNING "ASan runtime must be linked with Python or manually preloaded with LD_PRELOAD=/usr/lib/libasan.so")
endif()

# compiler and platform-dependent settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/target")
if (MSVC)
    add_compile_options(/MT /MTd /EHsc)
    add_compile_options("$<$<CONFIG:Release>:/O2>")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_BINARY_DIR}/target")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${CMAKE_BINARY_DIR}/target")
else()
    # -DELPP_THREAD_SAFE -- for easylogging++ thread safety
    set(BUILD_OPTS "-DELPP_THREAD_SAFE")
    if (BUILD_NATIVE)
        string(JOIN ";" BUILD_OPTS "${BUILD_OPTS}" "-march=native")
    endif()

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
            "-Wno-error" # Use of -Werror is discouraged with sanitizers
            # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
            "${ASAN_OPTS}")
        set(DEBUG_LINK_OPTS "${ASAN_OPTS}")
    elseif (UBSAN)
        # Set DEBUG build options specific for build with UBSAN
        string(JOIN ";" UBSAN_OPTS "-fsanitize=undefined"
            "-fsanitize=float-divide-by-zero"
            "-Wno-error" # Use of -Werror is discouraged with sanitizers
            # See https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003dbuiltin
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

    if (GDB_DEBUG)
        add_compile_options(-ggdb3)
    endif()

    # Workaround clang-18 bug:
    # https://github.com/llvm/llvm-project/issues/76515?ysclid=m406q4it5k674680045
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        string(FIND "${CMAKE_CXX_COMPILER_VERSION}" "18" IDX)
        if (IDX EQUAL 0)  # clang major version is 18
            message(WARNING "C++ compiler is Clang++-18. Supressing deprecated declaration warnings. Consider using another version of Clang")
            string(JOIN ";" DEBUG_BUILD_OPTS "${DEBUG_BUILD_OPTS}" "-Wno-deprecated-declarations")
        endif()
    endif()

    add_compile_options("$<$<CONFIG:Debug>:${DEBUG_BUILD_OPTS}>")
    add_link_options("$<$<CONFIG:Debug>:${DEBUG_LINK_OPTS}>")

    add_compile_options("$<$<CONFIG:Release>:${RELEASE_BUILD_OPTS}>")
endif()

# configuring boost
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.81.0 REQUIRED COMPONENTS container thread graph CONFIG)
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

include_directories(SYSTEM "lib/easyloggingpp/src" "lib/better-enums/" "lib/emhash" "lib/atomicbitvector/include" "lib/frozen/include")

# adding submodules
if (COMPILE_TESTS)
    add_subdirectory("lib/googletest" SYSTEM)
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

if (USE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

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
