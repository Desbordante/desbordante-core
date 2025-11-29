//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#include <gtest/gtest.h>

#include "util/logger.h"

int main(int argc, char** argv) {
    util::logging::EnsureInitialized();
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
