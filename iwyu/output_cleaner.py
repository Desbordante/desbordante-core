import sys
import os
import re
import json

def filter_iwyu_output(input_stream, main_output_stream, ignored_warnings_output_path, config_path=None):
    in_error_block = False
    previous_line_was_empty = False
    IGNORED_WARNING_PREFIX = "warning: no private include name for @headername mapping"
    recorded_unique_warnings = set()
    error_buffer = []
    
    in_ignored_block = False
    ignored_block_buffer = []
    
    iwyu_block_start = re.compile(r'.* should (add|remove) these lines:|The full include-list for .*')
    correct_include_pattern = re.compile(r'\(.* has correct #includes/fwd-decls\)')
    
    ignore_paths = []
    ignore_patterns = []
    
    if config_path and os.path.exists(config_path):
        try:
            with open(config_path, 'r') as f:
                config = json.load(f)
                ignore_paths = config.get('ignore_paths', [])
                ignore_patterns = config.get('ignore_patterns', [])
                print(f"Loaded config from {config_path}", file=sys.stderr)
        except Exception as e:
            print(f"Config load error: {e}", file=sys.stderr)
    else:
        print(f"Config file not found: {config_path}", file=sys.stderr)
    
    compiled_patterns = [re.compile(pattern) for pattern in ignore_patterns]

    try:
        ignored_output_file = open(ignored_warnings_output_path, 'w')
    except IOError as e:
        print(f"Error: Cannot open output file '{ignored_warnings_output_path}': {e}", file=sys.stderr)
        sys.exit(1)

    def should_ignore(line):
        for path in ignore_paths:
            if path in line:
                return True
        
        for pattern in compiled_patterns:
            if pattern.search(line):
                return True
        
        return False

    try:
        for line in input_stream:
            stripped_line = line.strip()

            if correct_include_pattern.match(stripped_line) and should_ignore(stripped_line):
                continue

            if iwyu_block_start.match(stripped_line):
                if should_ignore(stripped_line):
                    in_ignored_block = True
                    ignored_block_buffer.append(line)
                    continue
                else:
                    in_ignored_block = False

            if in_ignored_block:
                ignored_block_buffer.append(line)
                if stripped_line == "---":
                    ignored_block_buffer = []
                    in_ignored_block = False
                continue

            if stripped_line.startswith(("error:", "fatal error:")):
                in_error_block = True
                error_buffer.append(line)
                continue

            if in_error_block:
                if not stripped_line:
                    in_error_block = False
                    for error_line in error_buffer:
                        ignored_output_file.write(error_line)
                    ignored_output_file.write("\n")
                    error_buffer = []
                    continue
                else:
                    error_buffer.append(line)
                    continue

            if IGNORED_WARNING_PREFIX in stripped_line:
                if stripped_line not in recorded_unique_warnings:
                    ignored_output_file.write(line)
                    recorded_unique_warnings.add(stripped_line)
                continue

            if not stripped_line:
                if not previous_line_was_empty:
                    main_output_stream.write("\n")
                previous_line_was_empty = True
            else:
                main_output_stream.write(line)
                previous_line_was_empty = False

    finally:
        if error_buffer:
            for error_line in error_buffer:
                ignored_output_file.write(error_line)
            ignored_output_file.write("\n")
        
        if ignored_block_buffer:
            ignored_block_buffer = []
            
        ignored_output_file.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 output_cleaner.py <ignored_warnings_file> [config_path]", file=sys.stderr)
        sys.exit(1)
    
    ignored_file_path = sys.argv[1]
    config_path = sys.argv[2] if len(sys.argv) >= 3 else os.path.join(os.path.dirname(__file__), "iwyu_config.json")

    print(f"Filtering IWYU output...", file=sys.stderr)
    print(f"Configuration file: {config_path}", file=sys.stderr)
    print(f"Errors and unique warnings saved to: {ignored_file_path}", file=sys.stderr)

    filter_iwyu_output(sys.stdin, sys.stdout, ignored_file_path, config_path)
    print("Filtering completed.", file=sys.stderr)
