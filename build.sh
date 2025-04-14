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
  -u,         --no-unpack             Don't unpack datasets
  -j[N],      --parallel[N]           The maximum number of concurrent processes for building
  -d,         --debug                 Set debug build type
  -s[S],      --sanitizer[=S]         Build with sanitizer S (has effect only for debug build).
                                      Possible values of S: ADDRESS, UB.
                                      ADDRESS - Address Sanitizer
                                      UB      - Undefined Behavior Sanitizer
  -l                                  Use Link Time Optimization
  -g                                  Use GDB's debug information format
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
        # Display help
        -h | --help | *)
            print_help
            exit 0
            ;;
    esac
done

if [[ $NO_TESTS == true ]]; then
    PREFIX="$PREFIX -D COMPILE_TESTS=OFF"
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

rm -f build/CMakeCache.txt
cmake -S . -B build $PREFIX -G Ninja && cmake --build build $JOBS_OPTION
