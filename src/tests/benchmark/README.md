# Benchmark subsystem

## How to add benchmarks

All benchmarks must be placed must be placed in `src/tests/benchmark` (no subfolders)
for CMake to handle them correctly.

Consider you are implementing benchmark for `XX`-mining algorithm `XXMiner` on dataset
`huge_dataset.csv`, which has a `CSVConfig` named `kHugeDataset` in `all_csv_configs.h`.
Then you'll need to add file `src/tests/benchmark/XX_benchmark.h` with following contents:
```C++
#pragma once

// There's a singleton `BenchmarkController` that controls execution of all benchmarks.
// It lives in `src/tests/benchmark/benchmark_controller.h`:
#include "benchmark_controller.h"

void XXBenchmark {
    // Obtain an instance:
    auto& instance = tests::BenchmarkController::Instance();

    // Test body. Will be further discussed
}
```

Then this function must be added to `src/tests/benchmark/main.cpp`:
```C++
// line 33:
    for (auto test_register_func : {/* other tests */, XXBenchmark}) {
        test_register_func();
    }
```

### Simple benchmarks

Consider you want to add a test that simply executes algorithm.
Then `XXBenchmark` will look like this:
```C++
void XXBenchmark {
    // Obtain an instance:
    auto& instance = tests::BenchmarkController::Instance();
    algos::StdParamsMap params = {/* algo parameters */};
    // Arguments are: CSVConfig, other algo parameters, test name appendix and threshold
    instance.RegisterSimpleTest(kHugeDataset, std::move(params), "simple", 15 /* per cent */);
}
```

This will create test named "`XXMiner, huge_dataset, simple`" that creates `XXMiner` instance
with `params` parameters and calls `Execute`.

Name appendix is optional and can be used when you need several tests on one algo (for example,
using different error measures).

Time threshold for this test will be 15%. If not specified, it defaults to 10%.

For examples, see `fd_benchmark.h`.

### More complex benchmarks

Now, consider you want to do some preparation before calling `Execute`.
For this purpose `BenchmarkController` has `RegisterTest` method:
```C++
void XXBenchmark {
    // Obtain an instance:
    auto& instance = tests::BenchmarkController::Instance();
    auto test = [/* note that this object lives longer than XXBenchmark.
                 Don't capture things by reference */] {
        // Some preparation
        algo.Execute();
    };
    instance.RegisterTest(std::move(test), "XXMiner, huge_dataset", 15 /* per cent */);
}
```

This will create test named "`XXMiner, huge_dataset`", which executes `test`.

For example, see `md_benchmark.h`.

### Precise control on time measurement

If you *really need* more precise control on which part of test is measured (e. g. your algo has
a randomized step that you don't want to measure), you can use `RegisterCustomTest` method.
Its first parameter is executable, that measures time, saves results, compares them with
previous run on its own (see methods `EmplaceResult` and `GetPrevResults`) and returns 
`true` if time is good, and `false` otherwise.

`BenchmarkController` does no work now, except for calling you test.

You shouldn't use this way, until you really know what you're doing.

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
