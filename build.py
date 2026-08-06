#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "click>=8.2.0, <9",
# ]
# ///

import shlex
import subprocess
from pathlib import Path

import click


@click.command()
@click.option("-p", "--pybind", is_flag=True, help="Compile Python bindings")
@click.option("-n", "--no-tests", is_flag=True, help="Don't build tests")
@click.option("-b", "--benchmark", is_flag=True, help="Build benchmarks")
@click.option(
    "-j",
    "--parallel",
    type=int,
    metavar="N",
    help="Maximum number of concurrent build jobs",
)
@click.option("-d", "--debug", is_flag=True, help="Debug build")
@click.option(
    "-s",
    "--sanitizer",
    type=click.Choice(["ADDRESS", "UB"]),
    help="Build with sanitizer (has effect only for debug build)",
)
@click.option("-l", "--lto", is_flag=True, help="Enable link-time optimization")
@click.option(
    "-g",
    "--gdb-debug",
    is_flag=True,
    help="Use GDB debug information format",
)
@click.option(
    "-f",
    "--no-fetch-datasets",
    is_flag=True,
    help="Don't fetch datasets",
)
@click.option(
    "-L",
    "--log-level",
    type=click.Choice(["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"]),
    help="Set log level",
)
@click.option(
    "-C",
    "--cmake-opts",
    metavar="OPTS",
    help="Forward options to CMake",
)
@click.option(
    "-B",
    "--build-opts",
    metavar="OPTS",
    help="Forward options to the build system",
)
def main(
    pybind: bool,
    no_tests: bool,
    benchmark: bool,
    parallel: int | None,
    debug: bool,
    sanitizer: str | None,
    lto: bool,
    gdb_debug: bool,
    no_fetch_datasets: bool,
    log_level: str | None,
    cmake_opts: str,
    build_opts: str,
) -> None:
    cmake_args = ["-G", "Ninja"]

    if no_tests:
        cmake_args.append("-DDESBORDANTE_BUILD_TESTS=OFF")

    if benchmark:
        cmake_args.append("-DDESBORDANTE_BUILD_BENCHMARKS=ON")

    if pybind:
        cmake_args.append("-DDESBORDANTE_BINDINGS=BUILD")

    if lto:
        cmake_args.append("-DDESBORDANTE_USE_LTO=ON")

    if gdb_debug:
        cmake_args.append("-DDESBORDANTE_GDB_SYMBOLS=ON")

    if no_fetch_datasets:
        cmake_args.append("-DDESBORDANTE_FETCH_DATASETS=OFF")

    if not debug:
        cmake_args.append("-DCMAKE_BUILD_TYPE=Release")

    if sanitizer:
        cmake_args.append(f"-DDESBORDANTE_SANITIZER={sanitizer}")

    if log_level:
        cmake_args.append(f"-DDESBORDANTE_LOG_LEVEL={log_level}")

    if cmake_opts:
        cmake_args.extend(shlex.split(cmake_opts))

    build_args: list[str] = []

    if parallel is not None:
        build_args.extend(["-j", str(parallel)])

    if build_opts:
        build_args.extend(shlex.split(build_opts))

    Path("build/CMakeCache.txt").unlink(missing_ok=True)

    subprocess.run(
        ["cmake", "-S", ".", "-B", "build", *cmake_args],
        check=True,
    )

    subprocess.run(
        ["cmake", "--build", "build", *build_args],
        check=True,
    )


if __name__ == "__main__":
    main()
