#!/usr/bin/env python3
"""
IWYU Runner Script for Desbordante

This script runs the Include What You Use (IWYU) tool on the Desbordante project
with proper mapping files configuration.
"""

import argparse
import subprocess
import sys
from pathlib import Path


def get_script_dir() -> Path:
    """Get the directory where this script is located."""
    return Path(__file__).parent


def get_project_root() -> Path:
    """Get the project root directory (parent of iwyu folder)."""
    return get_script_dir().parent


def get_mappings_dir() -> Path:
    """Get the mappings directory."""
    return get_script_dir() / "mappings"


def check_dependencies() -> bool:
    """Check if required tools are available."""
    required_commands = ["iwyu_tool"]
    
    for cmd in required_commands:
        result = subprocess.run(["which", cmd], capture_output=True)
        if result.returncode != 0:
            print(f"Error: {cmd} not found in PATH", file=sys.stderr)
            return False
    return True


def check_build_directory() -> bool:
    """Check if build directory with compile_commands.json exists."""
    project_root = get_project_root()
    build_dir = project_root / "build"
    compile_commands = build_dir / "compile_commands.json"
    
    if not compile_commands.exists():
        print(f"Error: {compile_commands} not found.", file=sys.stderr)
        print("Please run 'cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON' first.", file=sys.stderr)
        return False
    return True


def rebuild_compile_commands() -> bool:
    """
    Regenerate compile_commands.json using cmake.
    
    This ensures IWYU has up-to-date compilation database.
    
    Returns:
        True if successful, False otherwise
    """
    project_root = get_project_root()
    build_dir = project_root / "build"
    
    print("Regenerating compile_commands.json...")
    
    # Run cmake to regenerate compile_commands.json
    # Using -DCMAKE_EXPORT_COMPILE_COMMANDS=ON to ensure compilation database is generated
    result = subprocess.run(
        ["cmake", "-S", ".", "-B", "build", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"],
        cwd=project_root,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        print(f"Error running cmake:", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        return False
    
    compile_commands = build_dir / "compile_commands.json"
    if not compile_commands.exists():
        print(f"Error: compile_commands.json still not found after rebuild.", file=sys.stderr)
        return False
    
    print(f"Successfully regenerated compile_commands.json")
    return True


def build_mapping_args() -> list[str]:
    """Build the mapping file arguments for iwyu_tool."""
    mappings_dir = get_mappings_dir()
    mapping_files = [
        "external.imp",
        "boost.imp",
        "boost-private.imp",
        "libc_include_map.imp",
        "libc_symbol_map.imp",
        "libcxx_include_map.imp",
        "libstdcpp_include_map.imp",
        "stdlib_c_include_map.imp",
        "stdlib_cpp_include_map.imp",
        "stdlib_cxx_symbol_map.imp",
    ]
    
    args = []
    for mapping_file in mapping_files:
        mapping_path = mappings_dir / mapping_file
        if mapping_path.exists():
            args.extend(["-Xiwyu", f"--mapping_file={mapping_path}"])
        else:
            print(f"Warning: Mapping file {mapping_path} not found, skipping", file=sys.stderr)
    
    return args


def run_iwyu(jobs: int, output_file: str | None = None, verbose: bool = False, rebuild: bool = False) -> int:
    """
    Run IWYU analysis on the project.
    
    Args:
        jobs: Number of parallel jobs
        output_file: Optional path to save output
        verbose: Enable verbose output
        rebuild: Whether to rebuild compile_commands.json before running IWYU
        
    Returns:
        Exit code from iwyu_tool
    """
    if not check_dependencies():
        return 1
    
    if not check_build_directory():
        return 1
    
    if rebuild:
        if not rebuild_compile_commands():
            print("Failed to rebuild compile_commands.json", file=sys.stderr)
            return 1
    
    project_root = get_project_root()
    build_dir = project_root / "build"
    
    # Build command
    cmd = [
        "iwyu_tool",
        "-p", str(build_dir),
        "-j", str(jobs),
        "-e", str(project_root / "cmake" / "cpm-cache"),
        "--",
        "-Xiwyu", "--no_fwd_decls",
    ]
    
    # Add --no_internal_mappings to disable IWYU's mappings
    # We use our own mapping files instead
    cmd.extend(["-Xiwyu", "--no_internal_mappings"])

    # Add mapping files
    cmd.extend(build_mapping_args())
    
    print(f"Running: {' '.join(cmd)}")
    print(f"Working directory: {project_root}")
    print(f"Using mappings from: {get_mappings_dir()}")
    print()
    
    # Run the command
    if output_file:
        print(f"Writing output to: {output_file}")
        with open(output_file, "w") as f:
            result = subprocess.run(cmd, cwd=project_root, stdout=f, stderr=subprocess.STDOUT)
        print(f"Done. Output saved to {output_file}")
        return result.returncode
    else:
        result = subprocess.run(cmd, cwd=project_root, capture_output=False)
        return result.returncode


def main():
    parser = argparse.ArgumentParser(
        description="Run IWYU analysis on Desbordante project",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # Run with default settings (4 jobs)
  %(prog)s -j 8                     # Run with 8 parallel jobs
  %(prog)s -o iwyu_output.txt       # Save output to file
  %(prog)s -r                       # Rebuild compile_commands.json before running
  %(prog)s -o out.txt -r            # Rebuild and save output
  %(prog)s -v                       # Verbose mode
        """
    )
    
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=4,
        help="Number of parallel jobs (default: 4)"
    )
    
    parser.add_argument(
        "-o", "--output",
        type=str,
        help="Path to save IWYU output"
    )
    
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output"
    )
    
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only check dependencies and exit"
    )
    
    parser.add_argument(
        "-r", "--rebuild",
        action="store_true",
        help="Rebuild compile_commands.json before running IWYU"
    )
    
    args = parser.parse_args()
    
    if args.check:
        print("Checking dependencies...")
        if check_dependencies():
            print("All dependencies found.")
        else:
            print("Some dependencies are missing.")
            return 1
        
        print("\nChecking build directory...")
        if check_build_directory():
            print("Build directory ready.")
        else:
            print("Build directory not ready.")
            return 1
        
        print(f"\nMappings directory: {get_mappings_dir()}")
        print(f"Available mapping files:")
        for f in get_mappings_dir().glob("*.imp"):
            print(f"  - {f.name}")
        
        return 0
    
    return run_iwyu(args.jobs, args.output, args.verbose, args.rebuild)


if __name__ == "__main__":
    sys.exit(main())
