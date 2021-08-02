#include "FDAlgorithm.h"
#include "json.hpp"

std::vector<std::string> FDAlgorithm::getColumnNames() {
    return inputGenerator_.getColumnNames();
}

std::string FDAlgorithm::getJsonFDs() {
    nlohmann::json j = nlohmann::json::array();

    fdCollection_.sort();
    for (auto& fd : fdCollection_) {
        j.push_back(fd.toJSON());
    }
    return j.dump();
}

std::string FDAlgorithm::getJsonArrayNameValue(int degree, bool withAttr) {
    size_t numberOfColumns = inputGenerator_.getNumberOfColumns();
    auto columnNames = inputGenerator_.getColumnNames();
    std::vector<double> LhsValues(numberOfColumns, 0);
    std::vector<double> RhsValues(numberOfColumns, 0);

    for (const auto &fd : fdCollection_) {
        double divisor = std::pow(fd.getLhs().getArity(), degree);

        const auto &LhsColumnIndices = fd.getLhs().getColumnIndices();
        for (size_t index = LhsColumnIndices.find_first();
            index != boost::dynamic_bitset<>::npos;
            index = LhsColumnIndices.find_next(index)) {
                LhsValues[index] += 1/divisor;
        }
        const auto &RhsColumn = fd.getRhs();
        size_t index = RhsColumn.getIndex();
        RhsValues[index] += 1/divisor;
    }
    nlohmann::json j;

    std::vector<std::pair<nlohmann::json, nlohmann::json>> lhs_array;
    std::vector<std::pair<nlohmann::json, nlohmann::json>> rhs_array;
    for (size_t i=0; i!= numberOfColumns; ++i) {
        auto name = withAttr ? columnNames[i] : std::to_string(i);
        lhs_array.push_back({{"name", name}, {"value", LhsValues[i]}});
        rhs_array.push_back({{"name", name}, {"value", RhsValues[i]}});
    }
    
    j["lhs"] = lhs_array;
    j["rhs"] = rhs_array;

    return j.dump();
}

unsigned int FDAlgorithm::fletcher16() {
    std::string toHash = getJsonFDs();
    unsigned int sum1 = 0, sum2 = 0, modulus = 255;
    for (auto ch : toHash) {
        sum1 = (sum1 + ch) % modulus;
        sum2 = (sum2 + sum1) % modulus;
    }
    return (sum2 << 8) | sum1;
}
