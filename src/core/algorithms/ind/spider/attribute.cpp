/** \file
 * \brief Spider attribute
 *
 * Attribute class methods definition
 */
#include "attribute.h"

namespace algos::spider {

std::vector<AttributeIndex> AINDAttribute::GetRefIds(config::ErrorType max_error) const {
    std::vector<AttributeIndex> refs;
    auto const dep_count = static_cast<double>(occurrences_[id_]);
    for (size_t ref_id = 0; ref_id != occurrences_.size(); ++ref_id) {
        if (id_ == ref_id) continue;
        config::ErrorType const error = 1 - occurrences_[ref_id] / dep_count;
        if (error <= max_error) {
            refs.push_back(ref_id);
        }
    }
    return refs;
}

boost::dynamic_bitset<> INDAttribute::GetBitset(AttributeIndex attr_id, AttributeIndex attr_count) {
    boost::dynamic_bitset<> bitset(attr_count);
    bitset.set(attr_id);
    bitset.flip();
    return bitset;
}

void INDAttribute::IntersectRefs(boost::dynamic_bitset<> const& bitset,
                                 std::vector<INDAttribute>& attrs) {
    for (auto ref_id : util::BitsetToIndices<AttributeIndex>(refs_ & ~bitset)) {
        attrs[ref_id].RemoveDependent(id_);
    }
    refs_ &= bitset;
}

}  // namespace algos::spider
