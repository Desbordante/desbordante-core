#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONFIG_FILE="${IWYU_CONFIG:-$SCRIPT_DIR/iwyu_config.json}"

show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Run IWYU analysis on a C++ project with configurable filtering.

OPTIONS:
    -h, --help              Show this help message
    -o, --output NAME       Set output file name (default: from config)
    -j, --jobs NUM          Set number of parallel jobs (default: from config)
    -c, --config FILE       Use alternative config file (default: $CONFIG_FILE)

EXAMPLES:
    $0                          # Use default settings from config
    $0 -o my_analysis -j 8      # Custom output name and job count
    $0 --output test_run --jobs 4
    $0 -c /path/to/custom_config.json

Configuration file: $CONFIG_FILE
EOF
}

# Default values
OUTPUT=""
NUM_JOBS=""
CUSTOM_CONFIG=""

# Parse command line arguments
while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -o|--output)
            OUTPUT="$2"
            shift 2
            ;;
        -j|--jobs)
            NUM_JOBS="$2"
            shift 2
            ;;
        -c|--config)
            CUSTOM_CONFIG="$2"
            shift 2
            ;;
        *)
            # Handle positional arguments for backward compatibility
            if [ -z "$OUTPUT" ]; then
                OUTPUT="$1"
            elif [ -z "$NUM_JOBS" ]; then
                NUM_JOBS="$1"
            else
                echo "Error: Unknown argument: $1"
                show_help
                exit 1
            fi
            shift
            ;;
    esac
done

# Use custom config if provided
if [ -n "$CUSTOM_CONFIG" ]; then
    CONFIG_FILE="$CUSTOM_CONFIG"
fi

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Config file not found: $CONFIG_FILE"
    exit 1
fi

CONFIG_DIR="$(dirname "$CONFIG_FILE")"

# Read basic config values
TARGET_PATH=$(jq -r '.target_path // "."' "$CONFIG_FILE")
RESULT_PATH=$(jq -r '.result_path // "."' "$CONFIG_FILE")
CONFIG_OUTPUT=$(jq -r '.output // "result_iwyu"' "$CONFIG_FILE")
CONFIG_JOBS=$(jq -r '.num_jobs // 4' "$CONFIG_FILE")

# Use command line values if provided, otherwise use config values
OUTPUT="${OUTPUT:-$CONFIG_OUTPUT}"
NUM_JOBS="${NUM_JOBS:-$CONFIG_JOBS}"

resolve_path() {
    local path="$1"
    if [ "${path#/}" = "$path" ]; then
        echo "$CONFIG_DIR/$path"
    else
        echo "$path"
    fi
}

TARGET_PATH=$(resolve_path "$TARGET_PATH")
RESULT_PATH=$(resolve_path "$RESULT_PATH")

mkdir -p "$RESULT_PATH/logs/"

cd "$TARGET_PATH" || { echo "Error: Cannot cd to $TARGET_PATH"; exit 1; }

BUILD_DIR="$TARGET_PATH/build"
COMPILE_COMMANDS_FILE="$BUILD_DIR/compile_commands.json"

if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists"
    if [ -f "$COMPILE_COMMANDS_FILE" ]; then
        echo "compile_commands.json found, skipping rebuild"
    else
        echo "compile_commands.json not found, rebuilding"
        rm -rf "$BUILD_DIR" || { echo "Error: Cannot remove $BUILD_DIR"; exit 1; }
        mkdir -p "$BUILD_DIR" || { echo "Error: Cannot create $BUILD_DIR"; exit 1; }
        cd "$BUILD_DIR" || { echo "Error: Cannot cd to $BUILD_DIR"; exit 1; }
        cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. || { echo "Error: CMake failed"; exit 1; }
        echo "Build directory and compile_commands.json recreated"
    fi
else
    echo "Build directory not found, creating"
    mkdir -p "$BUILD_DIR" || { echo "Error: Cannot create $BUILD_DIR"; exit 1; }
    cd "$BUILD_DIR" || { echo "Error: Cannot cd to $BUILD_DIR"; exit 1; }
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. || { echo "Error: CMake failed"; exit 1; }
    echo "Build directory and compile_commands.json created"
fi

cd "$BUILD_DIR" || { echo "Error: Cannot cd to $BUILD_DIR"; exit 1; }

generate_iwyu_args() {
    local args=""
    
    # Add mappings
    local mappings=$(jq -r '.mappings[]?' "$CONFIG_FILE" 2>/dev/null)
    if [ -n "$mappings" ]; then
        while read -r mapping; do
            mapping=$(resolve_path "$mapping")
            args="$args -Xiwyu --mapping_file=$mapping"
        done <<EOF
$mappings
EOF
    fi
    
    # Add flags
    local flags=$(jq -r '.flags[]?' "$CONFIG_FILE" 2>/dev/null)
    if [ -n "$flags" ]; then
        while read -r flag; do
            args="$args -Xiwyu $flag"
        done <<EOF
$flags
EOF
    fi
    
    # Add keep files
    local keep_files=$(jq -r '.keep_files[]?' "$CONFIG_FILE" 2>/dev/null)
    if [ -n "$keep_files" ]; then
        while read -r keep_file; do
            keep_file=$(resolve_path "$keep_file")
            args="$args -Xiwyu --keep=$keep_file"
        done <<EOF
$keep_files
EOF
    fi
    
    echo "$args"
}

IWYU_ARGS=$(generate_iwyu_args)

MAIN_LOG_FILE="$RESULT_PATH/logs/$OUTPUT.txt"
IGNORED_WARNINGS_FILE="$RESULT_PATH/logs/err_$OUTPUT.txt"

echo "Running iwyu_tool with $NUM_JOBS jobs..."
echo "Output file: $OUTPUT"
echo "IWYU arguments: $IWYU_ARGS"

iwyu_tool -j "$NUM_JOBS" -p . -- $IWYU_ARGS 2>&1 \
    | python3 "$RESULT_PATH/output_cleaner.py" "$IGNORED_WARNINGS_FILE" "$CONFIG_FILE" \
    > "$MAIN_LOG_FILE"

echo "IWYU analysis completed."
echo "Main report: $MAIN_LOG_FILE"
echo "Errors and unique ignored warnings: $IGNORED_WARNINGS_FILE"
