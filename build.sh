#!/bin/bash

function print_help() {
cat << EOF
Usage: ./build.sh [-h|--help] [-p|--pybind] [-t|--tests] [-j=N|--jobs=N]

-h,     --help,          Display help
-p,     --pybind,        Enable python bindings compile
-t,     --tests,         Enable tests build and datasets unpacking
-j[=N], --jobs[=N],      Allow N jobs at once. (Default [=1])

EOF
}

# Set default value for jobs
JOBS_AMOUNT=1

for i in "$@"
    do
    case $i in
        -p|--pybind) # Enable python bindings compile
            PYBIND=true
            ;;
        -t|--tests) # Enable tests build and datasets unpacking
            TESTS=true
            ;;
        -j=*|--jobs=*)
            JOBS_AMOUNT="${i#*=}"
            ;;
        -h|--help|*) # Display help.
            print_help
            exit 0
            ;;
    esac
done

mkdir -p lib
cd lib

git clone https://github.com/amrayn/easyloggingpp/ --branch v9.97.0 --depth 1
git clone https://github.com/aantron/better-enums.git --branch 0.11.3 --depth 1

if [[ $TESTS == true ]]; then
  git clone https://github.com/google/googletest/ --branch release-1.12.1 --depth 1
else
  PREFIX="$PREFIX -D ENABLE_TESTS_COMPILE=OFF"
fi
if [[ $PYBIND == true ]]; then
  git clone https://github.com/pybind/pybind11.git --branch v2.10 --depth 1
else
  PREFIX="$PREFIX -D ENABLE_PYBIND_COMPILE=OFF"
fi

cd ..
mkdir -p build
cd build
rm CMakeCache.txt
cmake $PREFIX .. "-DCMAKE_BUILD_TYPE=RELEASE"
make -j$JOBS_AMOUNT
