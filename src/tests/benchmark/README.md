# Benchmark subsystem

## How to add benchmarks

All benchmarks must be placed must be placed in `src/tests/benchmark` (no subfolders)
for CMake to handle them correctly.

Consider you are implementing benchmark for `XX`-mining algorithm `XXMiner` on dataset
`huge_dataset.csv`, which has a `CSVConfig` named `kHugeDataset` in `all_csv_configs.h`.
Then you'll need to add file `src/tests/benchmark/XX_benchmark.h` with following contents:
```C++
#pragma once

// There're `BenchmarkRunner` and `BenchmarkComparer` classes that control
// execution of all benchmarks.
// They live in `src/tests/benchmark/benchmark_runner.h` and
// `src/test/benchmark/benchmark_comparer.h`:
#include "benchmark_runner.h"
#include "benchmark_comparer.h"

void XXBenchmark(benchmark::BenchmarkRunner& runner, benchmark::BenchmarkComparer& comparer) {
    // Test body. Will be further discussed
}
```

Then this function must be added to `src/tests/benchmark/main.cpp`:
```C++
// line 33:
    for (auto test_register_func : {/* other tests */, XXBenchmark}) {
        test_register_func(runner, comparer);
    }
```

### Simple benchmarks

Consider you want to add a test that simply executes algorithm.
Then `XXBenchmark` will look like this:
```C++
void XXBenchmark(BenchmarkRunner& runner, BenchmarkComparer& comparer) {
    algos::StdParamsMap params = {/* algo parameters */};
    // Arguments are: CSVConfig, other algo parameters and test name suffix
    // Returns generated benchmark name
    auto bench_name = runner.RegisterSimpleBenchmark(kHugeDataset, std::move(params), "simple");
    // Next line is optional, default threshold is 10%
    // SetThreshold takes benchmark name and desired threshold
    comparer.SetThreshold(bench_name, 15);
}
```

This will create test named "`XXMiner, huge_dataset, simple`" that creates `XXMiner` instance
with `params` parameters and calls `Execute`.

Name suffix is optional and may be useful when you need several tests on one algo (for example,
using different error measures).

Time threshold for this test will be 15%. If not specified, it defaults to 10%.

For examples, see `fd_benchmark.h`.

### More complex benchmarks

Now, consider you want to do some preparation before calling `Execute`.
For this purpose `BenchmarkRunner` has `RegisterTest` method:
```C++
void XXBenchmark(BenchmarkRunner& runner, BenchmarkComparer& comparer) {
    auto test = [/* note that this object lives longer than XXBenchmark.
                 Don't capture things by reference */] {
        // Some preparation
        algo.Execute();
    };
    constexpr auto bench_name = "XXMiner, huge_dataset";
    runner.RegisterBenchmark(bench_name, std::move(test));
    comparer.SetThreshold(bench_name, 15);
}
```

This will create test named "`XXMiner, huge_dataset`", which executes `test`.

For example, see `md_benchmark.h`.

## Running benchmarks locally

If you want to run these tests locally for some reason, you should do the following:
1. Build benchmarks:
```bash
./build.sh --benchmark
```
2. Run them:
```bash
cd build/target
./Desbordante_benchmark
```
Optionally you can specify file with previous results to compare to and filename to save current results:
```bash
./Desbordante_benchmark old-results.json new-results.json
```

## Running benchmarks in CI

Benchmarks are being ran in CI every week on `main` branch.
Time plots can be downloaded in workflow's artifacts on GitHub, in the bottom of "Actions" page.
