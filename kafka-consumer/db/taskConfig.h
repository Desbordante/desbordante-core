#pragma once
#include <string>
#include <iostream>
#include "DBManager.h"

class taskConfig{
    std::string taskID;
    std::string algName;
    double errorPercent;
    char semicolon;
    std::string datasetPath;
    std::string datasetPathSource;
    bool hasHeader;
    unsigned int maxLHS;

public:
    taskConfig(const std::string taskID, std::string algName, double errorPercent, char semicolon, 
        std::string datasetPath, std::string datasetPathSource, bool hasHeader, unsigned int maxLHS)
        :   taskID(taskID), algName(algName), errorPercent(errorPercent), semicolon(semicolon),
            datasetPath(datasetPath), datasetPathSource(datasetPathSource), hasHeader(hasHeader), maxLHS(maxLHS) {}

    std::string getAlgName() { return algName; }
    std::string getTaskID() { return taskID; }
    double getErrorPercent() { return errorPercent; }
    char getSemicolon() { return semicolon; }
    std::string getDatasetPath() { return datasetPath; }
    std::string getDatasetPathSource() { return datasetPathSource; }
    bool getHasHeader() { return hasHeader; }
    unsigned int getMaxLHS() { return maxLHS; }

    std::ostream& writeInfo(std::ostream& os) {
        os << "Task ID -- " << taskID
           << ", algorithm name -- " << algName
           << ", error percent -- " << errorPercent
           << ", datasetPath -- " << datasetPath
           << ", semicolon -- '" << semicolon << "'"
           << ", hasHeader -- " << hasHeader
           << ", maxLHS -- " << maxLHS
           << ".\n";
        return os;
    }

    // Send a request to DB for status updating
    void updateStatus(DBManager& manager, std::string status) {
        try {
            std::string query = "UPDATE tasks SET status = '" + status + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's status in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB for progress updating
    void updateProgress(DBManager& manager, double progress) {
        try {
            std::string query = "UPDATE tasks SET progress = " + std::to_string(progress) + " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's progress in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with a set of FDs
    void updateFDs(DBManager& manager, const std::string& FDs) {
        try {
            std::string query = "UPDATE tasks SET FDs = '" + FDs + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's FDs in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array (data for pie chart for client)
    void updateJsonArrayNameValue(DBManager& manager, const std::string& jsonArrayNameValue) {
        try {
            std::string query = "UPDATE tasks SET JsonArrayNameValue = '" + jsonArrayNameValue;
            query += "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with sending data to DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

};