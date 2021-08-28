#pragma once
#include <string>
#include <iostream>

#include "DBManager.h"

class taskConfig{
    std::string taskID;
    std::string algName;
    double errorPercent;
    char separator;
    std::string datasetPath;
    bool hasHeader;
    unsigned int maxLHS;

public:
    taskConfig(std::string taskID, std::string algName, double errorPercent, char separator, 
               std::string datasetPath, bool hasHeader, unsigned int maxLHS)
        :   taskID(taskID), algName(algName), errorPercent(errorPercent), separator(separator),
            datasetPath(datasetPath), hasHeader(hasHeader), maxLHS(maxLHS) {}

    auto getAlgName()       const { return algName; }
    auto getTaskID()        const { return taskID; }
    auto getErrorPercent()  const { return errorPercent; }
    auto getSeparator()     const { return separator; }
    auto getDatasetPath()   const { return datasetPath; }
    auto getHasHeader()     const { return hasHeader; }
    auto getMaxLHS()        const { return maxLHS; }

    std::ostream& writeInfo(std::ostream& os) {
        os << "Task ID -- " << taskID
           << ", algorithm name -- " << algName
           << ", error percent -- " << errorPercent
           << ", datasetPath -- " << datasetPath
           << ", separator -- '" << separator << "'"
           << ", hasHeader -- " << hasHeader
           << ", maxLHS -- " << maxLHS
           << ".\n";
        return os;
    }

    // Send a request to DB for status updating
    void updateStatus(DBManager const& manager, std::string status) const {
        try {
            std::string query = "UPDATE tasks SET status = '" + status + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's status in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB for progress updating
    void updateProgress(DBManager const& manager, double progressPercent) const {
        try {
            std::string query = "UPDATE tasks SET progress = " + std::to_string(progressPercent) + " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's progress in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with a set of FDs
    void updateJsonFDs(DBManager const& manager, const std::string& FDs) const {
        try {
            std::string query = "UPDATE tasks SET FDs = '" + FDs + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's FDs in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array (data for pie chart for client)
    void updateJsonArrayNameValue(DBManager const& manager, const std::string& arrayNameValue) const {
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
    void updateJsonColumnNames(DBManager const& manager, const std::string& columnNames) const {
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
    void updateErrorStatus(DBManager const& manager, std::string const& error, std::string const& errorStatus) const {
        try {
            std::string query = "UPDATE tasks SET status = '" + error + "', errorStatus = '" + errorStatus + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's error status in DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    void setElapsedTime(DBManager const& manager, unsigned long long time) const {
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