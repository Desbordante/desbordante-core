# Performance testing subsystem

## How to add performance tests

All performance tests must be placed must be placed in `src/tests/performance_tests` (no subfolders)
for CMake to handle them correctly.

Consider you are implementing performance tests for `XX`-mining algorithm `XXMiner` on dataset
`huge_dataset.csv`, which has a `CSVConfig` named `kHugeDataset` in `all_csv_configs.h`.
Then you'll need to add file `src/tests/performance_tests/XX_performance_tests.h` with following
contents:
```C++
#pragma once

// There's a singleton `PerformanceTesting` that controls execution of all performance tests.
// It lives in `src/tests/performance_tests/performance_testing.h`:
#include "performance_testing.h"

void XXPerformanceTests {
    // Obtain an instance:
    auto& instance = tests::PerformanceTesting::Instance();

    // Test body. Will be further discussed
}
```

Then this function must be added to `src/tests/performance_tests/main.cpp`:
```C++
// line 33:
    for (auto test_register_func : {/* other tests */, XXPerformanceTests}) {
        test_register_func();
    }
```

### Simple tests

Consider you want to add a test that simply executes algorithm.
Then `XXPerformanceTests` will look like this:
```C++
void XXPerformanceTests {
    // Obtain an instance:
    auto& instance = tests::PerformanceTesting::Instance();
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

For examples, see `fd_performance_tests.h`.

### More complex test

Now, consider you want to do some preparation before calling `Execute`.
For this purpose `PerformanceTesting` has `RegisterTest` method:
```C++
void XXPerformanceTests {
    // Obtain an instance:
    auto& instance = tests::PerformanceTesting::Instance();
    auto test = [/* note that this object lives longer than XXPerformanceTests. Don't capture by reference */] {
        // Some preparation
        algo.Execute();
    };
    instance.RegisterTest(std::move(test), "XXMiner, huge_dataset", 15 /* per cent */);
}
```

This will create test named "`XXMiner, huge_dataset`", which executes `test`.

For example, see `md_performance_tests.h`.

### Precise control on time measurement

If you *really need* more precise control on which part of test is measured (e. g. your algo has
a randomized step that you don't want to measure), you can use `RegisterCustomTest` method.
Its first parameter is executable, that measures time, saves results, compares them with
previous run on its own (see methods `EmplaceResult` and `GetPrevResults`) and returns *truthy*
value (i. e. the one that can be explicilty casted to `true`) if time is good, and *falsy*
otherwise.

`PerformanceTesting` does no work now, except for calling you test.

You shouldn't use this way, until you really know what you're doing.

## How tests are executed

Performance tests are executed in a separate job in CI on a separate self-hosted runner.

This job turns on runner, executes all tests, saves results locally on a runner, builds plots,
deletes very old results, and turns off runner.
The latter action is executed always, except for situations when job was cancelled (manually or by
another concurrent job).
Therefore you **must** disable server manually when you hit "cancel workflow" button on GitHub.

This job runs automatically once a week (at midday on Saturday), and can be ran manually.

Plots can be found on GitHub in job's artifacts (below workflow graph).
