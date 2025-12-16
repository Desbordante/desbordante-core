#!/bin/bash

# Formatted with `shfmt -i 4 -ci --w build.sh`

# Stop on error:
set -e

function print_help() {
    cat <<EOF
Usage: ./build.sh [options]

Possible options:
  -h,         --help                  Display help
  -p,         --pybind                Compile python bindings
  -n,         --no-tests              Don't build tests
  -b          --benchmark             Build benchmarks
  -u,         --no-unpack             Don't unpack datasets
  -j[N],      --parallel[N]           The maximum number of concurrent processes for building
  -d,         --debug                 Set debug build type
  -s[S],      --sanitizer[=S]         Build with sanitizer S (has effect only for debug build).
                                      Possible values of S: ADDRESS, UB.
                                      ADDRESS - Address Sanitizer
                                      UB      - Undefined Behavior Sanitizer
  -l                                  Use Link Time Optimization
  -g                                  Use GDB's debug information format
  -L[LEVEL]   --log-level[=LEVEL]     Set log level (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
  -C[OPT]     --cmake-opt[=OPT]       Forward OPT to CMake
  -B[OPT]     --build-opt[=OPT]       Forward OPT to build system
EOF
}

# TODO: use getopts or something else instead of bash
for i in "$@"; do
    case $i in
        # Compile python bindings
        -p | --pybind)
            PYBIND=true
            ;;
        # Don't build tests
        -n | --no-tests)
            NO_TESTS=true
            ;;
		# Build benchmarks
        -b|--benchmark)
            BENCHMARK=true
            ;;
        # Don't unpack datasets
        -u | --no-unpack)
            NO_UNPACK=true
            ;;
        # The maximum number of concurrent processes for building
        -j* | --parallel*)
            BUILD_OPTS="$BUILD_OPTS $i"
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
        # Set log level, long option
        --log-level=*)
            LOG_LEVEL="${i#*=}"
            ;;
        # Set log level, short option
        -L*)
            LOG_LEVEL="${i#*L}"
            ;;
        # Forward option to CMake, long option
        --cmake-opt=*)
            CMAKE_OPTS="$CMAKE_OPTS ${i#*=}"
            ;;
        # Forward option to CMake, short option
        -C*)
            CMAKE_OPTS="$CMAKE_OPTS ${i#*C}"
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

CMAKE_OPTS="$CMAKE_OPTS -G Ninja"

if [[ $NO_TESTS == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D COMPILE_TESTS=OFF"
fi

if [[ $BENCHMARK == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D COMPILE_BENCHMARKS=ON"
fi

if [[ $NO_UNPACK == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D UNPACK_DATASETS=OFF"
fi

if [[ $PYBIND == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D PYTHON=COMPILE"
fi

if [[ $LTO == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D USE_LTO=ON"
fi

if [[ $GDB_DEBUG == true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D GDB_DEBUG=ON"
fi

if [[ $DEBUG_MODE != true ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D CMAKE_BUILD_TYPE=Release"
fi

if [[ -n $SANITIZER ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D SANITIZER=${SANITIZER}"
fi

if [[ -n $LOG_LEVEL ]]; then
    CMAKE_OPTS="$CMAKE_OPTS -D LOG_LEVEL=${LOG_LEVEL}"
fi

rm -f build/CMakeCache.txt
cmake -S . -B build $CMAKE_OPTS && cmake --build build $BUILD_OPTS
