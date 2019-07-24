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

class CSVParser {
private:
    std::ifstream source;
    char separator;
    bool hasNext;
    std::string nextLine;
    void getNext();

public:
    CSVParser(fs::path& path);
    CSVParser(fs::path& path, char separator);
    //bool isSameChar(char separator, char escape);
    std::vector<std::string> parseNext();
    bool getHasNext();
    char getSeparator();

};