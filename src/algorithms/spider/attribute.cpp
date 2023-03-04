#include "attribute.h"

namespace algos {

Attribute::Attribute(std::size_t id, std::size_t n_cols, StrCursor cursor,
                     std::vector<std::string> const& max_values)
    : id_(id), cursor_(std::move(cursor)) {
    for (std::size_t i = 0; i != n_cols; ++i) {
        if (GetID() == i) {
            continue;
        }
        if (max_values[GetID()] <= max_values[i]) {
            GetRefs().insert(i);
        }
        if (max_values[GetID()] >= max_values[i]) {
            GetDeps().insert(i);
        }
    }
}

void Attribute::IntersectRefs(SSet const& referenced_attrs_ids, AttrMap& attrs) {
    for (auto referenced_it = GetRefs().begin(); referenced_it != GetRefs().end();) {
        auto referenced = *referenced_it;
        if (referenced_attrs_ids.find(referenced) == std::end(referenced_attrs_ids)) {
            referenced_it = GetRefs().erase(referenced_it);
            attrs.at(referenced).RemoveDependent(GetID());
        } else {
            referenced_it++;
        }
    }
}
int Attribute::CompareID(std::size_t id_lhs, std::size_t id_rhs) {
    if (id_lhs > id_rhs) {
        return 1;
    } else if (id_lhs < id_rhs) {
        return -1;
    }
    return 0;
}

int Attribute::CompareTo(Attribute const& other) const {
    if (!GetCursor().HasNext() && !other.GetCursor().HasNext()) {
        return CompareID(GetID(), other.GetID());
    } else if (!GetCursor().HasNext()) {
        return 1;
    } else if (!other.GetCursor().HasNext()) {
        return -1;
    }

    int order = GetCursor().GetValue().compare(other.GetCursor().GetValue());
    if (order == 0) {
        return CompareID(GetID(), other.GetID());
    }
    return order;
}
}  // namespace algos
