#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

#include "config/tabular_data/input_table_type.h"
#include "parser/csv_parser/csv_parser.h"

namespace tests {

static auto const test_data_dir = std::filesystem::current_path() / "input_data";

struct Dataset {
    std::string name;
    size_t hash;
    char separator;
    bool has_header;
};

/// csv table configuration info to create an input table
struct TableConfig {
    std::string_view name;
    char separator;
    bool has_header;

    std::filesystem::path GetPath() const {
        return test_data_dir / name;
    }

    config::InputTable MakeInputTable() const {
        return std::make_shared<CSVParser>(GetPath(), separator, has_header);
    }
};

class LightDatasets {
public:
    static inline const std::array<Dataset, 11> datasets_ = {
            {{"CIPublicHighway10k.csv", 33398, ',', true},
             {"neighbors10k.csv", 43368, ',', true},
             {"WDC_astronomical.csv", 22281, ',', true},
             {"WDC_age.csv", 19620, ',', true},
             {"WDC_appearances.csv", 25827, ',', true},
             {"WDC_astrology.csv", 40815, ',', true},
             {"WDC_game.csv", 6418, ',', true},
             {"WDC_science.csv", 19620, ',', true},
             {"WDC_symbols.csv", 28289, ',', true},
             {"breast_cancer.csv", 15121, ',', true},
             {"WDC_kepler.csv", 63730, ',', true}}};

    // DEPRECATED -- just use
    // for (auto dataset : LightDatasets::datasets) { ... }
    static size_t DatasetQuantity() {
        return datasets_.size();
    }
    static std::string DatasetName(size_t i) {
        return datasets_[i].name;
    }
    static char Separator(size_t i) {
        return datasets_[i].separator;
    }
    static bool HasHeader(size_t i) {
        return datasets_[i].has_header;
    }
    static unsigned int Hash(size_t i) {
        return datasets_[i].hash;
    }
};

class HeavyDatasets {
public:
    static inline const std::array<Dataset, 6> datasets_ = {
            {{"adult.csv", 23075, ';', false},
             {"CIPublicHighway.csv", 13035, ',', true},
             {"EpicMeds.csv", 50218, '|', true},
             {"EpicVitals.csv", 2083, '|', true},
             {"iowa1kk.csv", 28573, ',', true},
             {"LegacyPayors.csv", 43612, '|', true}}};

    // DEPRECATED -- just use
    // for (auto dataset : HeavyDatasets::datasets) { ... }
    static size_t DatasetQuantity() {
        return datasets_.size();
    }
    static std::string DatasetName(size_t i) {
        return datasets_[i].name;
    }
    static char Separator(size_t i) {
        return datasets_[i].separator;
    }
    static bool HasHeader(size_t i) {
        return datasets_[i].has_header;
    }
    static unsigned int Hash(size_t i) {
        return datasets_[i].hash;
    }
};
