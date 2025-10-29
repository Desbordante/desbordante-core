#pragma once

#include <iosfwd>
#include <string>
#include <utility>

#include "fd/pyrocommon/model/confidence_interval.h"
#include "model/table/vertical.h"

class DependencyCandidate {
private:
    bool is_exact_;

public:
    model::ConfidenceInterval error_;
    Vertical vertical_;

    DependencyCandidate(Vertical vertical, model::ConfidenceInterval error, bool is_exact)
        : is_exact_(is_exact), error_(error), vertical_(std::move(vertical)) {}

    bool operator<(DependencyCandidate const& other) const;

    // TODO: implement if used
    // bool operator==(DependencyCandidate const& other) const;

    bool IsExact() const {
        return is_exact_ && error_.IsPoint();
    }

    // TODO: check if these comparators are used in a right way
    // using comparator = std::function<bool (DependencyCandidate const &, DependencyCandidate const
    // &)>;
    static bool ArityComparator(DependencyCandidate const&, DependencyCandidate const&);
    static bool MinErrorComparator(DependencyCandidate const&, DependencyCandidate const&);
    // static bool maxErrorComparator(DependencyCandidate const &, DependencyCandidate const &);
    static bool FullArityErrorComparator(DependencyCandidate const&, DependencyCandidate const&);
    static bool FullErrorArityComparator(DependencyCandidate const&, DependencyCandidate const&);

    explicit operator std::string() const {
        return "Candidate " + static_cast<std::string>(vertical_) +
               static_cast<std::string>(error_);
    }

    friend std::ostream& operator<<(std::ostream&, DependencyCandidate const&);
};
