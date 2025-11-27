//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#include <gtest/gtest.h>

#include "core/util/logger.h"

int main(int argc, char** argv) {
    util::logging::EnsureInitialized();
    ::testing::InitGoogleTest(&argc, argv);
    LOG_INFO("starting tests...");

    return RUN_ALL_TESTS();
}
