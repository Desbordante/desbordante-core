cmake_minimum_required(VERSION 3.10)
project(Desbordante)

option(COMPILE_PYBIND "Build the python bindings" OFF)
option(COMPILE_TESTS "Build tests" ON)
option(UNPACK_DATASETS "Unpack datasets" ON)
option(ASAN "Enable sanitizer" ON)

# By default select Debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

if (ASAN AND COMPILE_PYBIND)
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
    add_compile_options("$<$<CONFIG:Debug>:-O0;-DELPP_THREAD_SAFE;-g;-Wall;-Wextra;-Werror;-fno-omit-frame-pointer>")
    if (ASAN)
        add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address>")
        add_link_options("$<$<CONFIG:Debug>:-fsanitize=address>")
    endif()
    add_compile_options("$<$<CONFIG:Release>:-O3;-DELPP_THREAD_SAFE>")
endif()

# configuring boost
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.72.0 REQUIRED COMPONENTS container program_options thread)
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
if (COMPILE_PYBIND)
    add_subdirectory("lib/pybind11")
endif()

set( CMAKE_BUILD_TYPE_COPY "${CMAKE_BUILD_TYPE}" )
set( CMAKE_BUILD_TYPE "Release" )
option(build_static_lib "Build easyloggingpp as a static library" ON)
add_subdirectory("lib/easyloggingpp")
set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_COPY} )

add_subdirectory("src/core")

if (COMPILE_TESTS)
    add_subdirectory("src/tests")
endif()
if (UNPACK_DATASETS)
    add_subdirectory("datasets")
endif()
add_subdirectory("cfg")

if (COMPILE_PYBIND)
    add_subdirectory("src/python_bindings")
endif()
