#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace algos::cfd {

using AttributeIndex = int;

class RawCFD {
public:
    struct RawItem {
    public:
        AttributeIndex GetAttribute() const {
            return attribute;
        }

        std::optional<std::string> GetValue() const {
            return value;
        }

        std::string ToString() const;

        AttributeIndex attribute;         /* attribute column index */
        std::optional<std::string> value; /* pattern value is optional */

        bool operator==(RawItem const& other) const = default;
    };

    using RawItems = std::vector<RawItem>;

private:
    RawItems lhs_;
    RawItem rhs_;
    unsigned int support_ = 0;
    double confidence_ = 0.0;

public:
    explicit RawCFD(RawItems lhs, RawItem rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    RawCFD() = default;

    RawCFD(RawItems lhs, RawItem rhs, unsigned int support, double confidence)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)), support_(support), confidence_(confidence) {}

    std::string ToString() const;

    RawItems const& GetLhs() const noexcept {
        return lhs_;
    }

    RawItem const& GetRhs() const noexcept {
        return rhs_;
    }

    unsigned int GetSupport() const noexcept {
        return support_;
    }

    double GetConfidence() const noexcept {
        return confidence_;
    }

    bool operator==(RawCFD const& other) const = default;
};

}  // namespace algos::cfd
