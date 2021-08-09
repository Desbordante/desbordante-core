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
    bool hasHeader;
    unsigned int maxLHS;

public:
    taskConfig(const std::string taskID, std::string algName, double errorPercent, char semicolon, 
        std::string datasetPath, bool hasHeader, unsigned int maxLHS)
        :   taskID(taskID), algName(algName), errorPercent(errorPercent), semicolon(semicolon),
            datasetPath(datasetPath), hasHeader(hasHeader), maxLHS(maxLHS) {}

    std::string getAlgName() { return algName; }
    std::string getTaskID() { return taskID; }
    double getErrorPercent() { return errorPercent; }
    char getSemicolon() { return semicolon; }
    std::string getDatasetPath() { return datasetPath; }
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
    void updateProgress(DBManager& manager, double progressPercent) {
        try {
            std::string query = "UPDATE tasks SET progress = " + std::to_string(progressPercent) + " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's progress in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with a set of FDs
    void updateJsonFDs(DBManager& manager, const std::string& FDs) {
        try {
            std::string query = "UPDATE tasks SET FDs = '" + FDs + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's FDs in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array (data for pie chart for client)
    void updateJsonArrayNameValue(DBManager& manager, const std::string& arrayNameValue) {
        try {
            std::string query = "UPDATE tasks SET arrayNameValue = '" + arrayNameValue;
            query += "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with sending data to DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array of column names
    void updateJsonColumnNames(DBManager& manager, const std::string& columnNames) {
        try {
            std::string query = "UPDATE tasks SET columnNames = '" + columnNames;
            query += "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's attribute column names in the DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB for changing task's status to 'ERROR' and update errorStatus;
    void updateErrorStatus(DBManager& manager, const std::string& error, const std::string& errorStatus) {
        try {
            std::string query = "UPDATE tasks SET status = '" + error + "', errorStatus = '" + errorStatus + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's error status in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    void setElapsedTime(DBManager& manager, const unsigned long long time) {
        try {
            std::string query = "UPDATE tasks SET elapsedTime = " + std::to_string(time);
            query += " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with sending data to DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

};