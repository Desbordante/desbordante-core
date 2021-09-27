#include "FDAlgorithm.h"
#include "json.hpp"

std::vector<std::string> FDAlgorithm::getColumnNames() {
    return inputGenerator_.getColumnNames();
}

std::string FDAlgorithm::getJsonFDs(bool withNullLhs) {
    nlohmann::json j = nlohmann::json::array();

    fdCollection_.sort();
    for (auto& fd : fdCollection_) {
        if (withNullLhs) {
            j.push_back(fd.toJSON());
        } else {
            if (fd.getLhs().getArity() != 0)
                j.push_back(fd.toJSON());
        }
    }
    return j.dump();
}

std::string FDAlgorithm::getJsonArrayNameValue(int degree, bool withAttr) {
    size_t numberOfColumns = inputGenerator_.getNumberOfColumns();
    auto columnNames = inputGenerator_.getColumnNames();

    std::vector<std::pair<double, int>> LhsValues(numberOfColumns);
    std::vector<std::pair<double, int>> RhsValues(numberOfColumns);
    
    for (size_t i = 0; i != numberOfColumns; ++i) {
        LhsValues[i] = RhsValues[i] = { 0, i };
    }

    for (const auto &fd : fdCollection_) {
        double divisor = std::pow(fd.getLhs().getArity(), degree);

        const auto &LhsColumnIndices = fd.getLhs().getColumnIndices();
        for (size_t index = LhsColumnIndices.find_first();
            index != boost::dynamic_bitset<>::npos;
            index = LhsColumnIndices.find_next(index)) {
                LhsValues[index].first += 1/divisor;
        }
        size_t index = fd.getRhs().getIndex();

        if (divisor != 0)
            RhsValues[index].first += 1/divisor;
        else
            RhsValues[index].first = -1;
    }

    auto pair_greater = [](std::pair<double, int> a, std::pair<double, int> b) {
        return a.first > b.first;
    };

    std::sort(LhsValues.begin(), LhsValues.end(), pair_greater);
    std::sort(RhsValues.begin(), RhsValues.end(), pair_greater);

    nlohmann::json j;

    std::vector<std::pair<nlohmann::json, nlohmann::json>> lhs_array;
    std::vector<std::pair<nlohmann::json, nlohmann::json>> rhs_array;

    for (size_t i = 0; i != numberOfColumns; ++i) {
        auto name = withAttr ? columnNames[LhsValues[i].second] : std::string("Attribute " + i);
        if (LhsValues[i].first > 0) {
            lhs_array.push_back({{"name", name}, {"value", LhsValues[i].first}});
        }
        name = withAttr ? columnNames[RhsValues[i].second] : std::string("Attribute " + i);
        if (RhsValues[i].first > 0) {
            rhs_array.push_back({{"name", name}, {"value", RhsValues[i].first}});
        }
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

std::string FDAlgorithm::getJsonColumnNames() {
    nlohmann::json j = nlohmann::json(inputGenerator_.getColumnNames());
    return j.dump();
}
void FDAlgorithm::addProgress(double const val) noexcept {
        assert(val >= 0);
        std::scoped_lock lock(progress_mutex_);
        cur_phase_progress_ += val;
        assert(cur_phase_progress_ < 101);
}

void FDAlgorithm::setProgress(double const val) noexcept {
        assert(0 <= val && val < 101);
        std::scoped_lock lock(progress_mutex_);
        cur_phase_progress_ = val;
}

std::pair<uint8_t, double> FDAlgorithm::getProgress() const noexcept {
        std::scoped_lock lock(progress_mutex_);
        return std::make_pair(cur_phase_id, cur_phase_progress_);
}

void FDAlgorithm::toNextProgressPhase() noexcept {
    std::scoped_lock lock(progress_mutex_);
    ++cur_phase_id;
    assert(cur_phase_id < phase_names_.size());
    assert(cur_phase_progress_ >= 100);
    cur_phase_progress_ = 0;
}

