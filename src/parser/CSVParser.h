//
// Created by Ilya on 09.07.19.
//

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <experimental/filesystem>
#include <vector>

namespace fs = std::experimental::filesystem;
using std::vector, std::string;

class CSVParser {
private:
    std::ifstream source;
    char separator;
    bool hasNext;
    std::string nextLine;
    int numberOfColumns;
    vector<string> columnNames;
    string relationName;
    void getNext();

public:
    CSVParser(fs::path path);
    CSVParser(fs::path path, char separator);
    //bool isSameChar(char separator, char escape);
    vector<string> parseNext();
    bool getHasNext();
    char getSeparator();
    int getNumberOfColumns();
    string getColumnName(int index);
    string getRelationName();
};