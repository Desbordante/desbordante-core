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

#include "../db/DBManager.h"
#include "../db/taskConfig.h"

INITIALIZE_EASYLOGGINGPP

void process_task(taskConfig& task, DBManager& manager) {
    std::string alg = task.getAlgName();
    std::string datasetPath = task.getDatasetPath();
    char separator = task.getSemicolon();
    bool hasHeader = task.getHasHeader();
    // TODO: For what we use seed?
    int seed = 0;
    double error = task.getErrorPercent();
    unsigned int maxLHS = task.getMaxLHS();
    unsigned int parallelism = 0;

    el::Loggers::configureFromGlobal("logging.conf");
    
    std::unique_ptr<FDAlgorithm> algorithmInstance;
    if (alg == "Pyro") {
        algorithmInstance = std::make_unique<Pyro>(datasetPath, separator, hasHeader, seed, error, maxLHS, parallelism);
    } else if (alg == "TaneX"){
        algorithmInstance = std::make_unique<Tane>(datasetPath, separator, hasHeader, error, maxLHS);
    }
    try {
        task.updateStatus(manager, "IN PROCESS");
        task.updateProgress(manager, 10);

        algorithmInstance->execute();

        task.updateFDs(manager, algorithmInstance->getJsonFDs());
        task.updateJsonArrayNameValue(manager, algorithmInstance->getJsonArrayNameValue());

        return;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        throw e;
    }
    throw std::runtime_error("Unexpected behavior during task processed");
}