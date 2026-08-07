#include "core/algorithms/fd/tane/model/lattice_vertex.h"

#include "core/util/getting_ptr.h"

namespace model {

using boost::dynamic_bitset, std::vector, std::shared_ptr, std::make_shared, std::string;

bool LatticeVertex::ComesBeforeAndSharePrefixWith(LatticeVertex const& that) const {
    dynamic_bitset<> const& this_indices = vertical_;
    dynamic_bitset<> const& that_indices = that.vertical_;

    int this_index = this_indices.find_first();
    int that_index = that_indices.find_first();

    int arity = this_indices.count();
    for (int i = 0; i < arity - 1; i++) {
        if (this_index != that_index) return false;
        this_index = this_indices.find_next(this_index);
        that_index = that_indices.find_next(that_index);
    }

    return this_index < that_index;
}

bool LatticeVertex::operator>(LatticeVertex const& that) const {
    if (vertical_.count() != that.vertical_.count())
        return vertical_.count() > that.vertical_.count();

    dynamic_bitset<> const& this_indices = vertical_;
    int this_index = this_indices.find_first();
    dynamic_bitset<> const& that_indices = that.vertical_;
    int that_index = that_indices.find_first();

    int result;
    while (true) {
        result = this_index - that_index;
        if (result) return (result > 0);
        this_index = this_indices.find_next(this_index);
        that_index = that_indices.find_next(that_index);
    }
}

PositionListIndex const* LatticeVertex::GetPositionListIndex() const {
    return std::visit([](auto&& ptr) -> PositionListIndex const* { return util::GetPointer(ptr); },
                      position_list_index_);
}

PLIWithSingletons const* LatticeVertex::GetPositionListIndexWithSingletons() const {
    return std::visit(
            [](auto const& ptr) -> PLIWS const* {
                auto a = util::GetPointer(ptr);
                if constexpr (std::is_same_v<std::decay_t<decltype(a)>, PLIWithSingletons const*>) {
                    return a;
                } else {
                    assert(false);
                    __builtin_unreachable();
                }
            },
            position_list_index_);
}

}  // namespace model
