#include <iostream>
#include <string>
#include <json.hpp>

#include "processTask.h"
#include "consumerManager.h"

#include "../db/taskConfig.h"
#include "../db/DBManager.h"

void removeSpaces(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
}

void consumerStart(DBManager const& manager, cppkafka::Consumer* const consumer, std::string topic_name) {
    try {
        consumer->subscribe({ topic_name });

        std::cout << "% Reading messages from topic: " << topic_name << std::endl;

        auto running = true;

        while (running) {
            auto msg = consumer->poll();
            if (msg) {
                if (msg.get_error()) {
                    if (!msg.is_eof()) {
                        std::cout << "[+] Received error notification: " << msg.get_error() << std::endl;
                    }
                } else {
                    if (msg.get_key()) {
                        std::cout << msg.get_key() << std::endl;
                    }

                    auto payload = nlohmann::json::parse(msg.get_payload());
                    auto taskID = payload["taskID"];

                    auto query = "SELECT taskid, algname, errorpercent, separator, datasetpath, maxlhs, hasheader FROM tasks WHERE taskID = '" 
                                        + std::string(taskID) + "'";
                    auto rows = manager.defaultQuery(query);

                    if (rows.size() != 1) {
                        std::cout << "This task [" + std::string(taskID) + "] isn't in the database" << std::endl; 
                        throw std::runtime_error("Incorrect server work, task isn't added to the DB");
                    }

                    std::string datasetPath = rows[0]["datasetpath"].c_str();
                    std::string algName = rows[0]["algname"].c_str();
                    removeSpaces(datasetPath);
                    removeSpaces(algName);
                    
                    auto errorPercent = std::stod(rows[0]["errorpercent"].c_str());
                    auto hasHeader = std::string(rows[0]["hasheader"].c_str()) == "t" ? true : false;
                    unsigned int maxLHS = std::stoi(rows[0]["maxlhs"].c_str());
                    char separator = rows[0]["separator"].c_str()[0];

                    taskConfig task(taskID, algName, errorPercent, separator, datasetPath, hasHeader, maxLHS);
                    std::cout << "Config for task was created, information about task:\n";
                    task.writeInfo(std::cout);

                    try {
                        process_task(task, manager);
                    } catch (const std::exception& e) {
                        std::cout << "Task wasn't processed (skipped)" << std::endl;
                        task.updateErrorStatus(manager, "SERVER ERROR", e.what());
                        continue;
                    }

                    task.updateStatus(manager, "COMPLETED");
                    task.updateProgress(manager, 100);

                    std::cout << "Task with ID '" << std::string(taskID) << "' was successfully processed." << std::endl;
                    
                    consumer->commit(msg);
                }
            }
        }
    } catch (const cppkafka::ConsumerException& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
        throw e;
    }
}