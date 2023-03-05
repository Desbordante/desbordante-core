#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "model/cursor.h"

namespace algos::ind {

/* A class containing an iterator to traverse through attribute column
 * and holding candidates for UINDs.
 */
class Attribute {
public:
    using SSet = std::set<std::size_t>;
    using AttrMap = std::unordered_map<std::size_t, Attribute>;

private:
    std::size_t id_;
    StrCursor cursor_;
    SSet refs_, deps_;

public:
    Attribute(std::size_t id, std::size_t n_cols, StrCursor cursor,
              std::vector<std::string> const& max_values);
    std::size_t GetId() const {
        return id_;
    }
    StrCursor& GetCursor() {
        return cursor_;
    }
    StrCursor const& GetCursor() const {
        return cursor_;
    }
    const SSet& GetRefs() const {
        return refs_;
    }
    SSet& GetRefs() {
        return refs_;
    }
    const SSet& GetDeps() const {
        return deps_;
    }
    SSet& GetDeps() {
        return deps_;
    }

    void IntersectRefs(SSet const& referenced_attrs_ids, AttrMap& attrs);
    void RemoveDependent(std::size_t dep) {
        GetDeps().erase(dep);
    }
    bool HasFinished() const {
        return !GetCursor().HasNext() || (GetRefs().empty() && GetDeps().empty());
    }

    static int CompareId(std::size_t id_lhs, std::size_t id_rhs);
    int CompareTo(Attribute const& other) const;
};
}  // namespace algos::ind
