#pragma once

#include <vector>

#include "id_list.h"
#include "pattern.h"

namespace algos::cmspade {
class EquivalenceClass {
private:
    Pattern const* class_identifier_;
    IdList id_list_;
    std::vector<EquivalenceClass> class_members_;

public:
    EquivalenceClass(Pattern const* pattern, IdList id_list)
        : class_identifier_(pattern), id_list_(std::move(id_list)) {}

    ~EquivalenceClass() = default;

    EquivalenceClass(EquivalenceClass const&) = delete;
    EquivalenceClass& operator=(EquivalenceClass const&) = delete;

    EquivalenceClass(EquivalenceClass&& other) = default;
    EquivalenceClass& operator=(EquivalenceClass&& other) = default;

    Pattern const* GetClassIdentifier() const {
        return class_identifier_;
    }

    void SetClassIdentifier(Pattern const* pattern) {
        class_identifier_ = pattern;
    }

    IdList const& GetIdList() const {
        return id_list_;
    }

    IdList& GetIdList() {
        return id_list_;
    }

    void SetIdList(IdList id_list) {
        id_list_ = std::move(id_list);
    }

    std::vector<EquivalenceClass> const& GetClassMembers() const {
        return class_members_;
    }

    std::vector<EquivalenceClass>& GetClassMembers() {
        return class_members_;
    }

    void AddClassMember(EquivalenceClass member) {
        class_members_.emplace_back(std::move(member));
    }

    EquivalenceClass const& GetMember(std::size_t index) const {
        return class_members_.at(index);
    }

    int CompareTo(EquivalenceClass const& other) const {
        if (!class_identifier_ || !other.class_identifier_) return 0;
        return class_identifier_->CompareTo(*other.class_identifier_);
    }

    void Clear() {
        class_identifier_ = nullptr;
        id_list_.Clear();
        class_members_.clear();
    }
};
}  // namespace algos::cmspade