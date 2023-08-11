#!/bin/bash

function print_help() {
cat << EOF
Usage: ./build.sh [-h|--help] [-p|--pybind] [-n|--no-test]
                  [-u|--no-unpack] [-jN|--jobs=N] [-d|--debug]

  -h,         --help                  Display help
  -p,         --pybind                Compile python bindings
  -n,         --no-tests              Don't build tests
  -u,         --no-unpack             Don't unpack datasets
  -j[N],      --jobs[=N]              Allow N jobs at once (default [=1])
  -d,         --debug                 Set debug build type

EOF
}

for i in "$@"
    do
    case $i in
        -p|--pybind) # Enable python bindings compile
            PYBIND=true
            ;;
        -n|--no-tests) # Disable tests build
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
        -h|--help|*) # Display help.
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

if [[ $NO_TESTS == true ]]; then
  PREFIX="$PREFIX -D COMPILE_TESTS=OFF"
else
  if [[ ! -d "googletest" ]] ; then
    git clone https://github.com/google/googletest/ --branch v1.13.0 --depth 1
  fi
fi

if [[ $NO_UNPACK == true ]]; then
  PREFIX="$PREFIX -D UNPACK_DATASETS=OFF"
fi

if [[ $PYBIND == true ]]; then
  if [[ ! -d "pybind11" ]] ; then
      git clone https://github.com/pybind/pybind11.git --branch v2.10 --depth 1
  fi
  PREFIX="$PREFIX -D COMPILE_PYBIND=ON"
fi

if [[ $DEBUG_MODE != true ]]; then
  PREFIX="$PREFIX -D CMAKE_BUILD_TYPE=Release"
fi

cd ..
mkdir -p build
cd build
rm CMakeCache.txt
cmake $PREFIX ..
make $JOBS_OPTION
