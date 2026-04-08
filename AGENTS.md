# AGENTS.md

## Build Commands

```bash
# Full build with Python bindings
./build.sh --pybind

# Debug build with sanitizers
./build.sh --debug --sanitizer=ADDRESS   # or: --sanitizer=UB

# Parallel build
./build.sh -j8

# Build without fetching datasets (faster for repeated rebuilds)
./build.sh --no-fetch-datasets
```

## Running Tests

```bash
# Run tests, excluding heavy datasets
cd build/target && ./Desbordante_test --gtest_filter='*:-*HeavyDatasets*'

# Or use ctest
ctest --test-dir build --exclude-regex ".*HeavyDatasets.*" -j8
```

## Codestyle

- C++: `clang-format-22` (check with `git diff -U0 origin/main...HEAD | clang-format-diff-22 -p 1`)
- Python: `ruff check`
- Config: `.clang-tidy` for static analysis

## Architecture

- `src/core/` - Main C++ codebase with algorithm implementations (FD, IND, OD, etc.)
- `src/python_bindings/` - pybind11 bindings wrapping core library
- `src/tests/` - Google Test-based unit tests

## Environment Requirements

- CMake 3.25+, Ninja build system
- GCC 10+ or Clang 16+ or Apple Clang 15+
- Boost 1.85-1.86 or 1.88+ (matching your compiler)
- Python 3.8+ for bindings

## Testing on PR

CI runs with multiple compiler/sanitizer combinations:
- GCC, LLVM Clang, Apple Clang
- Release, Debug, Debug+ASAN, Debug+UBSAN

Heavy dataset tests are excluded from regular CI and run separately.

## Python Development

Install locally with:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install .
```

Then `import desbordante` works.