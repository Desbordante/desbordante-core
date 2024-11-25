#!/bin/bash

# Stop on error:
set -e

function print_help() {
cat << EOF
Usage: ./build.sh [options]

Possible options:
  -h,         --help                  Display help
  -p,         --pybind                Compile python bindings
  -n,         --no-tests              Don't build tests
  -u,         --no-unpack             Don't unpack datasets
  -j[N],      --jobs[=N]              Allow N jobs at once (default [=1])
  -d,         --debug                 Set debug build type
  -s[S],      --sanitizer[=S]         Build with sanitizer S (has effect only for debug build).
                                      Possible values of S: ADDRESS, UB.
                                      ADDRESS - Address Sanitizer
                                      UB      - Undefined Behavior Sanitizer
  -l                                  Use Link Time Optimization
  -g                                  Use GDB's debug information format
EOF
}

for i in "$@"
    do
    case $i in
        -p|--pybind) # Compile python bindings
            PYBIND=true
            ;;
        -n|--no-tests) # Don't build tests
            NO_TESTS=true
            ;;
        -u|--no-unpack) # Don't unpack datasets
            NO_UNPACK=true
            ;;
        -j*|--jobs=*) # Allow N jobs at once
            JOBS_OPTION=$i
            ;;
        -d|--debug) # Set debug build type
            DEBUG_MODE=true
            ;;
        # It's a nightmare, we should use getopts for args parsing or even use something else
        # instead of bash
        --sanitizer=*) # Build with sanitizer S, long option
            SANITIZER="${i#*=}"
            ;;
        -s*) # Build with sanitizer S, short option
            SANITIZER="${i#*s}"
            ;;
         -l)
            LTO=true
            ;;
         -g)
            GDB_DEBUG=true
            ;;
        -h|--help|*) # Display help
            print_help
            exit 0
            ;;
    esac
done

mkdir -p lib
cd lib

if [[ ! -d "easyloggingpp" ]] ; then
  git clone https://github.com/amrayn/easyloggingpp/ --branch v9.97.0 --depth 1
fi
if [[ ! -d "better-enums" ]] ; then
  git clone https://github.com/aantron/better-enums.git --branch 0.11.3 --depth 1
fi
if [[ ! -d "pybind11" ]] ; then
  git clone https://github.com/pybind/pybind11.git --branch v2.13.4 --depth 1
fi
if [[ ! -d "emhash" ]] ; then
  git clone https://github.com/ktprime/emhash.git --depth 1
fi
if [[ ! -d "atomicbitvector" ]] ; then
  git clone https://github.com/ekg/atomicbitvector.git --depth 1
fi
if [[ ! -d "frozen" ]] ; then
  git clone https://github.com/serge-sans-paille/frozen.git --depth 1
fi

if [[ $NO_TESTS == true ]]; then
  PREFIX="$PREFIX -D COMPILE_TESTS=OFF"
else
  if [[ ! -d "googletest" ]] ; then
    git clone https://github.com/google/googletest/ --branch v1.14.0 --depth 1
  fi
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
mkdir -p build
cd build
rm -f CMakeCache.txt
cmake $PREFIX .. && make $JOBS_OPTION
