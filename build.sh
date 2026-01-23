#!/usr/bin/env bash

# Formatted with `shfmt -i 4 -ci --w build.sh`

# Stop on error:
set -e

# Defaults
PRESET="Release"
EXTRA_CMAKE_OPTS=()
EXTRA_BUILD_OPTS=()
RUN_TESTS=false

function print_help() {
    cat <<EOF
Usage: ./build.sh [options]

Possible options:
  -h,         --help                  Display help
  -p,         --pybind                Build python bindings
  -n,         --no-tests              Don't build tests
  -b          --benchmark             Build benchmarks
  -j[N],      --parallel[N]           The maximum number of concurrent processes for building
  -d,         --debug                 Set debug build type
  -s,         --sanitizer             Build with sanitizers (with script only debug builds)
  -l                                  Use Link Time Optimization
  -g                                  Use GDB's debug information format
  -t          --run-tests             Run tests after building
  -f,         --no-fetch-datasets     Don't fetch datasets for tests or benchmarks
  -L[LEVEL]   --log-level[=LEVEL]     Set log level (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
  -C[OPT]     --cmake-opt[=OPT]       Forward OPT to CMake
  -B[OPT]     --build-opt[=OPT]       Forward OPT to build system

Presets available in CMakePresets.json:
  Release     - Release build (default)
  Debug       - Debug build
  DebugSan    - Debug build with sanitizers
EOF
}

# TODO: use getopts or something else instead of bash
function parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -b | --benchmark)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_BUILD_BENCHMARKS=ON)
                shift
                ;;
            -d | --debug)
                DEBUG=true
                shift
                ;;
            # Don't fetch datasets for tests or benchmarks
            -f | --no-fetch-datasets)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_FETCH_DATASETS=OFF)
                shift
                ;;
            -g | --gdb)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_GDB_SYMBOLS=ON)
                shift
                ;;
            -h | --help)
                print_help
                exit 0
                ;;
            -l | --lto)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_USE_LTO=ON)
                shift
                ;;
            -n | --no-tests)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_BUILD_TESTS=OFF)
                shift
                ;;
            -p | --pybind)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_BINDINGS=BUILD)
                shift
                ;;
            -s | --sanitizer)
                SANITIZER=true
                shift
                ;;
            -t | --test)
                RUN_TESTS=true
                shift
                ;;
            -u | --no-unpack)
                EXTRA_CMAKE_OPTS+=(-D DESBORDANTE_UNPACK_DATASETS=OFF)
                shift
                ;;

            -j* | --parallel*)
                if [[ "$1" == *=* ]]; then
                    val="${1#*=}"
                    shift
                elif [[ "$1" == -j* ]] && [[ "${#1}" -gt 2 ]]; then
                    val="${1#-j}"
                    shift
                else
                    val="$2"
                    shift 2
                fi
                if [[ -n "$val" ]]; then
                    EXTRA_BUILD_OPTS+=("-j" "$val")
                else
                    echo "Error: -j|--parallel requires an argument" >&2
                    exit 1
                fi
                ;;

            -L* | --log-level=*)
                if [[ "$1" == *=* ]]; then
                    val="${1#*=}"
                    shift
                else
                    val="$2"
                    shift 2
                fi
                if [[ -n "$val" ]]; then
                    EXTRA_CMAKE_OPTS+=("-D" "DESBORDANTE_LOG_LEVEL=$val")
                else
                    echo "Error: -L|--log-level requires an argument" >&2
                    exit 1
                fi
                ;;

            -C* | --cmake-opt=*)
                if [[ "$1" == *=* ]]; then
                    val="${1#*=}"
                    shift
                else
                    val="$2"
                    shift 2
                fi
                if [[ -n "$val" ]]; then
                    EXTRA_CMAKE_OPTS+=("$val")
                else
                    echo "Error: -C|--cmake-opt requires an argument" >&2
                    exit 1
                fi
                ;;

            -B* | --build-opt*)
                if [[ "$1" == *=* ]]; then
                    val="${1#*=}"
                    shift
                else
                    val="$2"
                    shift 2
                fi
                if [[ -n "$val" ]]; then
                    EXTRA_BUILD_OPTS+=("$val")
                else
                    echo "Error: -B|--build-opt requires an argument" >&2
                    exit 1
                fi
                ;;

            *)
                echo "Error: Unknown option: $1" >&2
                print_help
                exit 1
                ;;
        esac
    done
}

function select_preset() {
    if [[ ${DEBUG:-false} == true ]]; then
        if [[ ${SANITIZER:-false} == true ]]; then
            PRESET="DebugSan"
        else
            PRESET="Debug"
        fi
    else
        if [[ ${SANITIZER:-false} == true ]]; then
            echo "Warning: Sanitizers are only supported in debug builds with this script, ignoring --sanitizer flag" >&2
        fi
        PRESET="Release"
    fi
}

function configure_and_build() {
    echo "Building with preset: $PRESET"
    echo "CMake opts: ${EXTRA_CMAKE_OPTS[*]}"
    echo "Build opts: ${EXTRA_BUILD_OPTS[*]}"

    cmake --preset="$PRESET" "${EXTRA_CMAKE_OPTS[@]}"
    cmake --build --preset="$PRESET" "${EXTRA_BUILD_OPTS[@]}"
}

function main() {
    parse_args "$@"

    select_preset
    configure_and_build "$PRESET"
    if [[ $RUN_TESTS == true ]]; then
        ctest --preset="$PRESET"
    fi
}

main "$@"
