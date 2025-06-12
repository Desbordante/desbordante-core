#pragma once

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "types.h"

namespace model {

struct NeARIDs {
    std::vector<FeatureIndex> ante;
    Consequence cons;
    double p_value = -1.0;

    NeARIDs() = default;

    NeARIDs(std::vector<FeatureIndex> ante, Consequence cons)
        : ante(std::move(ante)), cons(std::move(cons)) {}

    NeARIDs(std::vector<OFeatureIndex> ante, OConsequence cons,
            std::vector<FeatureIndex> const& order) {
        this->ante.reserve(ante.size());
        for (OFeatureIndex ante_i : ante) {
            this->ante.emplace_back(order[ante_i]);
        }
        this->cons.feature = order[cons.feature];
        this->cons.positive = cons.positive;
    }

    std::string ToString() const {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(3) << p_value << "  ";
        oss << std::defaultfloat;
        oss << "{";
        for (size_t i = 0; i < ante.size(); ++i) {
            oss << ante[i];
            if (i + 1 < ante.size()) oss << ", ";
        }
        oss << "} -> ";
        if (!cons.positive) oss << "not ";
        oss << cons.feature;
        return oss.str();
    }
};

struct NeARStrings {
    std::vector<std::string> ante;
    std::string cons;

    std::string ToString() const {
        std::ostringstream oss;

        for (size_t i = 0; i < ante.size(); ++i) {
            if (i != 0) oss << ", ";
            oss << ante[i];
        }
        oss << " -> " << cons;

        return oss.str();
    }
};

}  // namespace model