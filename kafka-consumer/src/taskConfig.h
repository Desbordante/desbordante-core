#pragma once
#include <string>
#include <iostream>

struct taskConfig{
    std::string taskID;
    std::string algName;
    double errorPercent;
    char semicolon;
    std::string datasetPath;
    std::string datasetPathSource;
    bool hasHeader;
    unsigned int maxLHS;

    taskConfig(const std::string& taskID, std::string algName, double errorPercent, char semicolon, 
        std::string datasetPath, std::string datasetPathSource, bool hasHeader, unsigned int maxLHS)
        :   taskID(taskID), algName(algName), errorPercent(errorPercent), semicolon(semicolon),
            datasetPath(datasetPath), datasetPathSource(datasetPathSource), hasHeader(hasHeader), maxLHS(maxLHS) {}

    std::ostream& writeInfo(std::ostream& os) {
        os << "Task ID -- " << taskID;
        os << ", algorithm name -- " << algName;
        os << ", error percent -- " << errorPercent;
        os << ", datasetPath -- " << datasetPath;
        os << ", semicolon -- '" << semicolon << "'";
        os << ", hasHeader -- " << hasHeader;
        os << ", maxLHS -- " << maxLHS;
        os << ".\n";
        return os;
    }
};