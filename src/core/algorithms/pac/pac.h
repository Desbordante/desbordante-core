#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/model/table/relational_schema.h"

namespace model {

/// @brief Probabilistic Approximate Constraints are dependencies that generalize exact dependencies.
/// Each PAC has two common parameters: list of epsilons and list of deltas (separate for each
/// attribute, or the same for all attributes).
/// Epsilon is a measure of "proximity", which shows how much PAC deviates from exact
/// dependency. Delta is a probability at which approximate dependency holds.
class PAC {
private:
    std::shared_ptr<RelationalSchema const> rel_schema_;
    std::vector<double> epsilons_;
    std::vector<double> deltas_;

public:
    PAC() = default;

    PAC(std::shared_ptr<RelationalSchema const> rel_schema, double epsilon, double delta)
        : rel_schema_(std::move(rel_schema)), epsilons_({epsilon}), deltas_({delta}) {}

    PAC(std::shared_ptr<RelationalSchema const> rel_schema, std::vector<double>&& epsilons,
        std::vector<double>&& deltas)
        : rel_schema_(std::move(rel_schema)),
          epsilons_(std::move(epsilons)),
          deltas_(std::move(deltas)) {}

    PAC(PAC const&) = default;
    PAC(PAC&&) = default;
    PAC& operator=(PAC const&) = default;
    PAC& operator=(PAC&&) = default;

    virtual ~PAC() = default;

    virtual std::string ToShortString() const = 0;
    virtual std::string ToLongString() const = 0;

    double GetEpsilon() const {
        if (epsilons_.size() != 1) {
            throw std::logic_error("Cannot get epsilon, because PAC has " +
                                   std::to_string(epsilons_.size()) + " different epsilons");
        }
        return epsilons_.front();
    }

    std::vector<double> const& GetEpsilons() const {
        return epsilons_;
    }

    void SetEpsilon(double const new_value) {
        SetEpsilons({new_value});
    }

    void SetEpsilons(std::vector<double>&& new_values) {
        epsilons_ = std::move(new_values);
    }

    double GetDelta() const {
        if (deltas_.size() != 1) {
            throw std::logic_error("Cannot get delta, because PAC has " +
                                   std::to_string(epsilons_.size()) + " different deltas");
        }
        return deltas_.front();
    }

    std::vector<double> const& GetDeltas() const {
        return deltas_;
    }

    void SetDelta(double const new_value) {
        SetDeltas({new_value});
    }

    void SetDeltas(std::vector<double>&& new_values) {
        deltas_ = std::move(new_values);
    }

    std::shared_ptr<RelationalSchema const> GetRelSchema() const {
        return rel_schema_;
    }
};
}  // namespace model
