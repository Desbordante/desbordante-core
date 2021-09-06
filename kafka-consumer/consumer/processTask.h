#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <boost/program_options.hpp>

#include "logging/easylogging++.h"

#include "algorithms/Pyro.h"
#include "algorithms/TaneX.h"
#include "algorithms/FastFDs.h"
#include "algorithms/DFD/DFD.h"
#include "algorithms/Fd_mine.h"

#include "../db/DBManager.h"
#include "../db/taskConfig.h"

INITIALIZE_EASYLOGGINGPP

void process_task(taskConfig const& task, DBManager const& manager) {
    auto alg = task.getAlgName();
    auto datasetPath = task.getDatasetPath();
    auto separator = task.getSeparator();
    auto hasHeader = task.getHasHeader();
    auto error = task.getErrorPercent();
    auto maxLHS = task.getMaxLHS();
    
    auto seed = 0;
    unsigned int parallelism = 0;

    el::Loggers::configureFromGlobal("logging.conf");
    
    std::unique_ptr<FDAlgorithm> algorithmInstance;

    if (alg == "Pyro") {
        algorithmInstance = std::make_unique<Pyro>(datasetPath, separator, hasHeader, seed, error, maxLHS, parallelism);
    } else if (alg == "TaneX") {
        algorithmInstance = std::make_unique<Tane>(datasetPath, separator, hasHeader, error, maxLHS);
    } else if (alg == "FastFDs") {
        algorithmInstance = std::make_unique<FastFDs>(datasetPath, separator, hasHeader);
    } else if (alg == "FD mine") {
        algorithmInstance = std::make_unique<Fd_mine>(datasetPath, separator, hasHeader);
    } else if (alg == "DFD") {
        algorithmInstance = std::make_unique<DFD>(datasetPath, separator, hasHeader);
    }

    try {
        task.updateStatus(manager, "IN PROCESS");
        
        unsigned long long elapsedTime;

        if (alg == "FastFDs") {
            std::thread progress_listener([&task, &manager, &algorithmInstance] {
                double progress = 0;
                while(progress != 100) {
                    progress = algorithmInstance->getProgress();
                    task.updateProgress(manager, progress);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
            elapsedTime = algorithmInstance->execute();
            progress_listener.join();
        } else {
            elapsedTime = algorithmInstance->execute();
            task.updateProgress(manager, 100);
        }

        task.setElapsedTime(manager, elapsedTime);
        task.updateJsonFDs(manager, algorithmInstance->getJsonFDs(false));
        task.updateJsonArrayNameValue(manager, algorithmInstance->getJsonArrayNameValue());
        task.updateJsonColumnNames(manager, algorithmInstance->getJsonColumnNames());
        task.updateStatus(manager, "COMPLETED");
        return;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
        throw e;
    }
    throw std::runtime_error("Unexpected behavior during task executing");
}