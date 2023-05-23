#pragma once

#include <filesystem>
#include <string>
#include <vector>

static const auto test_data_dir = std::filesystem::current_path() / "input_data";

struct Dataset {
    std::string name;
    size_t hash;
    char separator;
    bool header_presence;
};

class LightDatasets {
public:
    static inline const std::array<Dataset, 11> datasets_ = {
            {{"CIPublicHighway10k.csv", 21537, ',', true},
             {"neighbors10k.csv", 62210, ',', true},
             {"WDC_astronomical.csv", 49417, ',', true},
             {"WDC_age.csv", 44913, ',', true},
             {"WDC_appearances.csv", 34497, ',', true},
             {"WDC_astrology.csv", 12109, ',', true},
             {"WDC_game.csv", 32103, ',', true},
             {"WDC_science.csv", 44913, ',', true},
             {"WDC_symbols.csv", 2160, ',', true},
             {"breast_cancer.csv", 58641, ',', true},
             {"WDC_kepler.csv", 6284, ',', true}}};

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
        return datasets_[i].header_presence;
    }
    static unsigned int Hash(size_t i) {
        return datasets_[i].hash;
    }
};

class HeavyDatasets {
public:
    static inline const std::array<Dataset, 6> datasets_ = {
            {{"adult.csv", 20873, ';', false},
             {"CIPublicHighway.csv", 32696, ',', true},
             {"EpicMeds.csv", 19617, '|', true},
             {"EpicVitals.csv", 24082, '|', true},
             {"iowa1kk.csv", 55285, ',', true},
             {"LegacyPayors.csv", 34874, '|', true}}};

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
        return datasets_[i].header_presence;
    }
    static unsigned int Hash(size_t i) {
        return datasets_[i].hash;
    }
};
