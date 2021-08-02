#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "logging/easylogging++.h"

#include "algorithms/Pyro.h"
#include "algorithms/TaneX.h"

#include "db/DBManager.h"
#include "taskConfig.h"

INITIALIZE_EASYLOGGINGPP

void process_task(taskConfig& task, DBManager& manager) {
    std::string alg = task.algName;
    std::string datasetPath = task.datasetPath;
    char separator = task.semicolon;
    bool hasHeader = task.hasHeader;
    int seed = 0;
    double error = task.errorPercent;
    unsigned int maxLHS = task.maxLHS;
    unsigned int parallelism = 0;

    el::Loggers::configureFromGlobal("logging.conf");

    // "FastFDs", "Pyro", "TaneX"

    std::cout << "Input: algorithm \"" << alg
              << "\" with seed " << std::to_string(seed)
              << ", error \"" << std::to_string(error)
              << ", maxLHS \"" << std::to_string(maxLHS)
              << "\" and dataset path \"" << datasetPath
              << "\" with separator \'" << separator
              << "\'. Header is " << (hasHeader ? "" : "not ") << "present. " << std::endl;
    
    std::unique_ptr<FDAlgorithm> algorithmInstance;
    if (alg == "Pyro") {
        algorithmInstance = std::make_unique<Pyro>(datasetPath, separator, hasHeader, seed, error, maxLHS, parallelism);
    } else if (alg == "TaneX"){
        algorithmInstance = std::make_unique<Tane>(datasetPath, separator, hasHeader, error, maxLHS);
    }
    try {
        std::string query = "UPDATE tasks SET status = 'IN PROCESS' WHERE taskID = '" + std::string(task.taskID) + "'";
        try {
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cout << "Unexpected exception (with changing task's status in DB) caught: " << e.what() << std::endl;
            throw e;
        }
        unsigned long long elapsedTime = algorithmInstance->execute();

        std::string FDs = algorithmInstance->getJsonFDs();
        std::cout << FDs << std::endl;

        try {
            query = "UPDATE tasks SET FDs = '" + FDs + "' WHERE taskID = '" + std::string(task.taskID) + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cout << "Unexpected exception (with changing task's FDs in DB) caught: " << e.what() << std::endl;
            throw e;
        }
        std::cout << "> ELAPSED TIME: " << elapsedTime << std::endl;
        return;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        throw e;
    }
    throw std::runtime_error("Error while task was executed");
}