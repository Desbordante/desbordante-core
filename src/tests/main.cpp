//
// Created by Ilya Vologin
// https://github.com/cupertank
//

#include <easylogging++.h>
#include <gtest/gtest.h>

INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv) {
    el::Loggers::configureFromGlobal("logging.conf");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
