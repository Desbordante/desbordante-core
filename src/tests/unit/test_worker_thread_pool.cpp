#include <gtest/gtest.h>

#include "core/util/worker_thread_pool.h"

namespace tests {

TEST(WorkerThreadPool, ConstructAndDestroyWithoutWork) {
    util::WorkerThreadPool pool{4};
}

TEST(WorkerThreadPool, RepeatedConstructAndDestroyWithoutWork) {
    for (int i = 0; i < 100; ++i) {
        util::WorkerThreadPool pool{4};
    }
}

}  // namespace tests
