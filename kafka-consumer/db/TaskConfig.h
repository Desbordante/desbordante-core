#pragma once
#include <string>
#include <algorithm>
#include <iostream>

#include "DBManager.h"

class TaskConfig{
    std::string const taskID;
    std::string const algName;
    double const errorPercent;
    char const separator;
    std::string const datasetPath;
    bool const hasHeader;
    unsigned int const maxLHS;

    static std::string tableName;

    TaskConfig(std::string taskID, std::string algName, double errorPercent, 
               char separator, std::string datasetPath, bool hasHeader, 
               unsigned int maxLHS)
        :   taskID(taskID), algName(algName), errorPercent(errorPercent), 
            separator(separator), datasetPath(datasetPath), hasHeader(hasHeader), 
            maxLHS(maxLHS) {}
public:

    auto getAlgName()      const { return algName;      }
    auto getTaskID()       const { return taskID;       }
    auto getErrorPercent() const { return errorPercent; }
    auto getSeparator()    const { return separator;    }
    auto getDatasetPath()  const { return datasetPath;  }
    auto getHasHeader()    const { return hasHeader;    }
    auto getMaxLHS()       const { return maxLHS;       }

    auto& writeInfo(std::ostream& os) const {
        os << "Task Config:\n"
           << "ID -- " << taskID
           << ", algorithm name -- " << algName
           << ", error percent -- " << errorPercent
           << ", datasetPath -- " << datasetPath
           << ", separator -- '" << separator << "'"
           << ", hasHeader -- " << hasHeader
           << ", maxLHS -- " << maxLHS
           << ".\n";
        return os;
    }

    static void prepareString(std::string& str) {
        for(
            auto pos = std::find(str.begin(), str.end(), '\''); 
            pos != str.end(); 
            pos = std::find(pos, str.end(), '\'')
        ) {
            pos = str.insert(pos + 1, '\'');
            pos++;
        }
    }

    static bool taskExists(DBManager const &manager, std::string taskID) {
        std::string query = "SELECT COUNT(*) FROM " + tableName + 
                            " WHERE taskID = '" + taskID + "'";
        auto answer = manager.defaultQuery(query);
        return answer.size() == 1;
    }

    static TaskConfig getTaskConfig(DBManager const &manager, std::string taskID) {
        std::string query = "SELECT taskid, trim(algname) as algname, errorpercent,\n"
                            "separator, datasetpath, maxlhs, hasheader\n"
                            "FROM " + tableName + " WHERE taskID = '" + taskID + "'";
        auto rows = manager.defaultQuery(query);
        
        auto algName      = rows[0]["algname"].c_str();
        auto errorPercent = std::stod(rows[0]["errorpercent"].c_str());
        char separator    = rows[0]["separator"].c_str()[0];
        auto datasetPath  = rows[0]["datasetpath"].c_str();
        auto hasHeader    = rows[0]["hasheader"].c_str() == "t";
        auto maxLHS       = (unsigned int)std::stoi(rows[0]["maxlhs"].c_str());

        return TaskConfig(taskID, algName, errorPercent, separator, datasetPath, 
                          hasHeader, maxLHS);
    }

    // Send a request to DB for status updating
    void updateStatus(DBManager const &manager, std::string status) const {
        try {
            prepareString(status);
            std::string query = "UPDATE " + tableName + " SET status = "
                                "'" + status + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's status in DB) "
                      << "caught: " << e.what() << std::endl;
            throw e;
        }
    }

    void setMaxPhase(DBManager const& manager, size_t maxPhase) const {
        try {
            std::string query = "UPDATE " + tableName  + " SET maxPhase = "
                + std::to_string(maxPhase) + " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing maxPhase in DB) "
                      << "caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB for progress updating
    void updateProgress(DBManager const &manager, double progressPercent, 
        std::string const &phaseName = "", size_t curPhase = 0) const {
        try {
            std::string query = "UPDATE " + tableName + " SET progress = " 
                + std::to_string(progressPercent);
            if (phaseName.length()) {
                query += ", phaseName = '" + phaseName + "'";
            }
            if (curPhase != 0) {
                query += ", currentPhase = " + std::to_string(curPhase);
            }
            query += " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's progress in DB)"
                      << " caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with a set of FDs
    void updateJsonFDs(DBManager const& manager, const std::string& FDs) const {
        try {
            std::string query = "UPDATE " + tableName + " SET FDs = '" + FDs + "'"
                                " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's FDs in DB)"
                      << " caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array (data for pie chart for client)
    void updateJsonArrayNameValue(DBManager const& manager, 
                                  const std::string& arrayNameValue) const {
        try {
            std::string query = "UPDATE " + tableName + " SET arrayNameValue = '" 
                                + arrayNameValue + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with sending data to DB)"
                      << " caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB with JSON array of column names
    void updateJsonColumnNames(DBManager const& manager, 
                               const std::string& columnNames) const {
        try {
            std::string query = "UPDATE " + tableName + " SET columnNames = '" 
                                + columnNames + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with updating task's attribute "
                      << "column names in the DB) caught: " << e.what() << std::endl;
            throw e;
        }
    }

    // Send a request to DB for changing task's status to 'ERROR' 
    // and update errorStatus;
    void updateErrorStatus(DBManager const& manager, std::string error, 
                           std::string errorStatus) const {
        try {
            prepareString(error);
            prepareString(errorStatus);
            std::string query = "UPDATE " + tableName + " SET status = "
                                + "'" + error + "', errorStatus = '" + errorStatus
                                + "' WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with changing task's error status in DB)"
                      << " caught: " << e.what() << std::endl;
            throw e;
        }
    }

    void setElapsedTime(DBManager const& manager, unsigned long long time) const {
        try {
            std::string query = "UPDATE " + tableName + " SET elapsedTime = " 
                + std::to_string(time) + " WHERE taskID = '" + taskID + "'";
            manager.transactionQuery(query);
        } catch(const std::exception& e) {
            std::cerr << "Unexpected exception (with sending data to DB) "
                      << "caught: " << e.what() << std::endl;
            throw e;
        }
    }
};