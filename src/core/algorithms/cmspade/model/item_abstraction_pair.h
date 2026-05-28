#pragma once

#include "item.h"

namespace algos::cmspade {
class ItemAbstractionPair {
private:
    Item item_;
    bool have_equal_relation_;

public:
    ItemAbstractionPair(Item item, bool have_equal_relation)
        : item_(std::move(item)), have_equal_relation_(have_equal_relation) {}

    ItemAbstractionPair() : item_(0), have_equal_relation_(false) {}

    ~ItemAbstractionPair() = default;

    ItemAbstractionPair(ItemAbstractionPair const& other) = default;
    ItemAbstractionPair& operator=(ItemAbstractionPair const& other) = default;

    ItemAbstractionPair(ItemAbstractionPair&& other) noexcept = default;
    ItemAbstractionPair& operator=(ItemAbstractionPair&& other) noexcept = default;

    Item const& GetItem() const {
        return item_;
    }

    bool HaveEqualRelation() const {
        return have_equal_relation_;
    }

    bool Equals(ItemAbstractionPair const& other) const {
        return (item_ == other.item_) && (have_equal_relation_ == other.have_equal_relation_);
    }

    int CompareTo(ItemAbstractionPair const& other) const {
        if (item_ < other.item_) return -1;
        if (other.item_ < item_) return 1;

        if (!have_equal_relation_ && other.have_equal_relation_) return -1;
        if (have_equal_relation_ && !other.have_equal_relation_) return 1;

        return 0;
    }

    bool operator==(ItemAbstractionPair const& other) const {
        return Equals(other);
    }

    bool operator!=(ItemAbstractionPair const& other) const {
        return !Equals(other);
    }

    bool operator<(ItemAbstractionPair const& other) const {
        return CompareTo(other) < 0;
    }
};
}  // namespace algos::cmspade