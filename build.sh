#!/bin/bash

# Formatted with `shfmt -i 4 -ci --w build.sh`

# Stop on error:
set -e

function print_help() {
    cat <<EOF
Usage: ./build.sh [options]

Possible options:
  -h,         --help                  Display help
              --deps-only             Install dependencies only (don't build)
  -p,         --pybind                Compile python bindings
  -n,         --no-tests              Don't build tests
  -T          --perf-tests            Build performance tests
  -u,         --no-unpack             Don't unpack datasets
  -j[N],      --parallel[N]           The maximum number of concurrent processes for building
  -d,         --debug                 Set debug build type
  -s[S],      --sanitizer[=S]         Build with sanitizer S (has effect only for debug build).
                                      Possible values of S: ADDRESS, UB.
                                      ADDRESS - Address Sanitizer
                                      UB      - Undefined Behavior Sanitizer
  -l                                  Use Link Time Optimization
  -g                                  Use GDB's debug information format
  -C[OPT]     --cmake-opt[=OPT]       Forward OPT to CMake
  -B[OPT]     --build-opt[=OPT]       Forward OPT to build system
EOF
}

# TODO: use getopts or something else instead of bash
for i in "$@"; do
    case $i in
        # Install dependencies only (don't build)
        --deps-only)
            DEPS_ONLY=true
            ;;
        # Compile python bindings
        -p | --pybind)
            PYBIND=true
            ;;
        # Don't build tests
        -n | --no-tests)
            NO_TESTS=true
            ;;
        -T|--perf-tests) # Build performance tests
            PERF_TESTS=true
            ;;
        # Don't unpack datasets
        -u | --no-unpack)
            NO_UNPACK=true
            ;;
        # The maximum number of concurrent processes for building
        -j* | --parallel*)
            JOBS_OPTION=$i
            ;;
        # Set debug build type
        -d | --debug)
            DEBUG_MODE=true
            ;;
        # Build with sanitizer S, long option
        --sanitizer=*)
            SANITIZER="${i#*=}"
            ;;
        # Build with sanitizer S, short option
        -s*)
            SANITIZER="${i#*s}"
            ;;
        # Use Link Time Optimization
        -l)
            LTO=true
            ;;
        # Use GDB's debug information format
        -g)
            GDB_DEBUG=true
            ;;
        # Forward option to CMake, long option
        --cmake-opt=*)
            PREFIX="$PREFIX ${i#*=}"
            ;;
        # Forward option to CMake, short option
        -C*)
            PREFIX="$PREFIX ${i#*C}"
            ;;
        # Forward option to build system, long option
        --build-opt=*)
            BUILD_OPTS="$BUILD_OPTS ${i#*=}"
            ;;
        # Forward option to build system, short option
        -B*)
            BUILD_OPTS="$BUILD_OPTS ${i#*B}"
            ;;
        # Display help
        -h | --help | *)
            print_help
            exit 0
            ;;
    esac
done

mkdir -p lib
cd lib

if [[ ! -d "easyloggingpp" ]]; then
    git clone https://github.com/amrayn/easyloggingpp/ --branch v9.97.0 --depth 1
fi
if [[ ! -d "better-enums" ]]; then
    git clone https://github.com/aantron/better-enums.git --branch 0.11.3 --depth 1
fi
if [[ ! -d "pybind11" ]]; then
    git clone https://github.com/pybind/pybind11.git --branch v2.13.4 --depth 1
fi
if [[ ! -d "emhash" ]]; then
    git clone https://github.com/ktprime/emhash.git --depth 1
fi
if [[ ! -d "atomicbitvector" ]]; then
    git clone https://github.com/ekg/atomicbitvector.git --depth 1
fi
if [[ ! -d "frozen" ]]; then
    git clone https://github.com/serge-sans-paille/frozen.git --depth 1
fi

if [[ $NO_TESTS == true ]]; then
    PREFIX="$PREFIX -D COMPILE_TESTS=OFF"
else
    if [[ ! -d "googletest" ]]; then
        git clone https://github.com/google/googletest/ --branch v1.14.0 --depth 1
    fi
fi

if [[ $PERF_TESTS == true ]]; then
    PREFIX="$PREFIX -D COMPILE_PERFORMANCE_TESTS=ON"
fi

if [[ $DEPS_ONLY == true ]]; then
    exit 0
fi

if [[ $NO_UNPACK == true ]]; then
    PREFIX="$PREFIX -D UNPACK_DATASETS=OFF"
fi

if [[ $PYBIND == true ]]; then
    PREFIX="$PREFIX -D PYTHON=COMPILE -D COPY_PYTHON_EXAMPLES=ON"
fi

if [[ $LTO == true ]]; then
    PREFIX="$PREFIX -D USE_LTO=ON"
fi

if [[ $GDB_DEBUG == true ]]; then
    PREFIX="$PREFIX -D GDB_DEBUG=ON"
fi

if [[ $DEBUG_MODE != true ]]; then
    PREFIX="$PREFIX -D CMAKE_BUILD_TYPE=Release"
fi

if [[ -n $SANITIZER ]]; then
    PREFIX="$PREFIX -D SANITIZER=${SANITIZER}"
fi

cd ..
rm -f build/CMakeCache.txt
cmake -S . -B build $PREFIX -G Ninja && cmake --build build $JOBS_OPTION $BUILD_OPTS
