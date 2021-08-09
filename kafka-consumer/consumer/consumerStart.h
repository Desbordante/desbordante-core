#include <iostream>
#include <string>
#include <json.hpp>

#include "kafka/KafkaConsumer.h"
#include "processTask.h"

#include "../db/taskConfig.h"
#include "../db/DBManager.h"

void getNewRecordInfo(const kafka::ConsumerRecord &record, bool isFull = false) {
    std::cout << "% Got a new message..." << std::endl;
    std::cout << "    Topic    : " << record.topic() << std::endl;
    std::cout << "    Partition: " << record.partition() << std::endl;
    std::cout << "    Offset   : " << record.offset() << std::endl;
    if (isFull) {
        std::cout << "    Timestamp: " << record.timestamp().toString() << std::endl;
        std::cout << "    Headers  : " << kafka::toString(record.headers()) << std::endl;
        std::cout << "    Key   [" << record.key().toString() << "]" << std::endl;
    }
    std::cout << "    Value [" << record.value().toString() << "]" << std::endl;
}

void removeSpaces(std::string& str) {
    str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
}

void tasksConsumerStart(DBManager& manager, kafka::KafkaManualCommitConsumer& consumer, kafka::Topic topic) {
    try {
        // Subscribe to topics
        consumer.subscribe({topic});

        auto lastTimeCommitted = std::chrono::steady_clock::now();

        // Read messages from the topic.
        std::cout << "% Reading messages from topic: " << topic << std::endl;
        bool allCommitted = true;
        bool running      = true;

        while (running) {
            auto records = consumer.poll(std::chrono::milliseconds(100));
            for (const auto& record: records) {

                //  quit on empty message
                if (record.value().size() == 0) {
                    running = false;
                    break;
                }

                if (!record.error()) {
                    getNewRecordInfo(record);
                    // get task's JSON -- {"taskID":"4f829810-f192-11eb-96ec-9b0a4d5939a1"}
                    nlohmann::json task_json = nlohmann::json::parse(record.value().toString());
                    auto taskID = task_json["taskID"];

                    std::string query = "SELECT taskid, algname, errorpercent, semicolon, datasetpath, maxlhs, hasheader FROM tasks WHERE taskID = '" 
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
                    
                    double errorPercent = std::stod(rows[0]["errorpercent"].c_str());
                    unsigned int maxLHS = std::stoi(rows[0]["maxlhs"].c_str());
                    bool hasHeader = std::string(rows[0]["hasheader"].c_str()) == "t" ? true : false;
                    char semicolon = rows[0]["semicolon"].c_str()[0];

                    taskConfig task(taskID, algName, errorPercent, semicolon, datasetPath, hasHeader, maxLHS);
                    std::cout << "Config file for task was created, information about task:\n";
                    task.writeInfo(std::cout);

                    try {
                        process_task(task, manager);
                    } catch (const std::exception& e) {
                        std::cout << "Task wasn't processed (skipped)" << std::endl;
                        task.updateErrorStatus(manager, "SERVER ERROR", e.what()); // TODO
                        continue;
                    }

                    task.updateStatus(manager, "COMPLETED");
                    task.updateProgress(manager, 100);

                    std::cout << "Task with ID '" << std::string(taskID) << "' was successfully processed." << std::endl;
                    
                    allCommitted = false;
                } else {
                    std::cerr << record.toString() << std::endl;
                }
            }

            if (!allCommitted) {
                auto now = std::chrono::steady_clock::now();
                if (now - lastTimeCommitted > std::chrono::seconds(1)) {
                    // Commit offsets for messages polled
                    std::cout << "% syncCommit offsets: " << kafka::Utility::getCurrentTime() << std::endl;
                    consumer.commitSync(); // or commitAsync()

                    lastTimeCommitted = now;
                    allCommitted      = true;
                }
            }
        }

    } catch (const kafka::KafkaException& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
        throw e;
    }
}