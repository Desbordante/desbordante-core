#pragma once

#include <cstddef>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "core/algorithms/fd/fd.h"
#include "core/model/table/column.h"
#include "core/model/table/vertical.h"

namespace algos::cfd::cfun {
class Condition {
private:
    std::vector<std::string> pattern_;
    size_t support_;

public:
    Condition(std::vector<std::string> pattern, size_t support)
        : pattern_(std::move(pattern)), support_(support) {}

    std::vector<std::string> const& GetPattern() const noexcept {
        return pattern_;
    }

    size_t GetSupport() const noexcept {
        return support_;
    }

    bool operator==(Condition const& other) const = default;
};

class CCFD {
public:
    using Condition = cfun::Condition;
    using Tableau = std::vector<Condition>;

private:
    FD embedded_fd_;
    Tableau tableau_;
    size_t support_;

public:
    CCFD(Vertical lhs, Column rhs, Tableau tableau, size_t support,
         std::shared_ptr<RelationalSchema const> schema)
        : embedded_fd_(std::move(lhs), std::move(rhs), std::move(schema)),
          tableau_(std::move(tableau)),
          support_(support) {}

    CCFD(FD embedded_fd, Tableau tableau, size_t support)
        : embedded_fd_(std::move(embedded_fd)), tableau_(std::move(tableau)), support_(support) {}

    bool operator==(CCFD const& other) const {
        return support_ == other.support_ && tableau_ == other.tableau_ &&
               embedded_fd_.ToNameTuple() == other.embedded_fd_.ToNameTuple();
    }

    FD const& GetEmbeddedFd() const noexcept {
        return embedded_fd_;
    }

    Tableau const& GetTableau() const noexcept {
        return tableau_;
    }

    size_t GetSupport() const noexcept {
        return support_;
    }

    std::string ToString() const {
        std::ostringstream os;
        os << embedded_fd_.ToLongString() << "\n";

        for (auto const& condition : tableau_) {
            auto const& pattern = condition.GetPattern();
            if (pattern.empty()) {
                continue;
            }

            os << "\t";

            for (size_t i = 0; i < pattern.size() - 1; ++i) {
                os << "." << pattern[i] << ".";
            }

            os << "| (" << pattern.back() << ")\n";
        }
        return os.str();
    }
};
}  // namespace algos::cfd::cfun
