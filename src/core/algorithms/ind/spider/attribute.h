/** \file
 * \brief Spider attribute
 *
 * Attribute class definition
 */
#pragma once

#include <string>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "config/error/type.h"
#include "model/table/column_combination.h"
#include "model/table/column_domain_iterator.h"
#include "model/table/column_index.h"
#include "table/column_domain.h"
#include "util/bitset_utils.h"

namespace algos::spider {

using AttributeIndex = model::ColumnIndex;

namespace details {
/// base class for IND attributes
class Attribute {
public:
    using Iterator = model::ColumnDomainIterator;

protected:
    AttributeIndex id_;         /* attribute unique identificator */
    AttributeIndex attr_count_; /* attribute unique identificator */
    Iterator it_;               /* domain iterator */

public:
    Attribute(AttributeIndex attr_id, AttributeIndex attr_count, model::ColumnDomain const& domain)
        : id_(attr_id), attr_count_(attr_count), it_(domain) {}

    /// get unqiue attribute id
    AttributeIndex GetId() const noexcept {
        return id_;
    }

    /// check whether the attribute has processed
    virtual bool HasFinished() const noexcept {
        return !it_.HasNext();
    }

    std::string const& GetCurrentValue() const noexcept {
        return it_.GetValue();
    }

    void MoveToNext() {
        it_.MoveToNext();
    }

    /// compare attributes first by their values and then by their ids
    bool operator>(Attribute const& rhs) const {
        int const cmp = GetCurrentValue().compare(rhs.GetCurrentValue());
        return cmp == 0 ? GetId() > rhs.GetId() : cmp > 0;
    }

    model::ColumnCombination ToCC() const {
        model::ColumnDomain const& domain = it_.GetDomain();
        return {domain.GetTableId(), std::vector{domain.GetColumnId()}};
    }
};
}  // namespace details

/// attribute for AIND
class AINDAttribute final : public details::Attribute {
    std::vector<unsigned int> occurrences_;

public:
    template <typename... Args>
    explicit AINDAttribute(Args&&... args)
        : details::Attribute(std::forward<Args>(args)...), occurrences_(attr_count_) {}

    void IntersectRefs(std::vector<AttributeIndex> const& ids) {
        for (auto ref_id : ids) {
            occurrences_[ref_id]++;
        }
    }

    /// get referenced attribute indices
    std::vector<AttributeIndex> GetRefIds(config::ErrorType max_error) const;

    /// get error threshold
    config::ErrorType GetError(AttributeIndex ref_id) const noexcept {
        auto const dep_count = static_cast<config::ErrorType>(occurrences_[id_]);
        return 1 - occurrences_[ref_id] / dep_count;
    }
};

/// attribute for IND
class INDAttribute final : public details::Attribute {
    boost::dynamic_bitset<> refs_; /* referenced attributes indices */
    boost::dynamic_bitset<> deps_; /* dependent attributes indices */

    static boost::dynamic_bitset<> GetBitset(AttributeIndex attr_id, AttributeIndex attr_count);

public:
    template <typename... Args>
    explicit INDAttribute(Args&&... args)
        : details::Attribute(std::forward<Args>(args)...),
          refs_(GetBitset(id_, attr_count_)),
          deps_(refs_) {}

    ///
    /// \brief intersect referenced attributes indices with provided indices
    ///
    /// Iterate through the referenced attributes indices and remove those
    /// that do not exist in the provided indices `bitset`.\n
    /// Additionally, it updates dependent attributes indices.
    ///
    /// \param bitset    indices bitset to intersect with
    /// \param attrs     attribute objects to update
    ///
    void IntersectRefs(boost::dynamic_bitset<> const& bitset, std::vector<INDAttribute>& attrs);

    /// get referenced attribute indices
    std::vector<AttributeIndex> GetRefIds() const {
        return util::BitsetToIndices<AttributeIndex>(refs_);
    }

    /// remove dependent attribute index
    void RemoveDependent(AttributeIndex id) {
        deps_.set(id, false);
    }

    ///
    /// \brief check whether the attribute has processed
    ///
    /// processing is completed if all values have been processed
    /// or there are no more dependent and referenced candidates
    ///
    bool HasFinished() const noexcept final {
        return !it_.HasNext() || (refs_.none() && deps_.none());
    }

    /// get referenced attributes indices
    boost::dynamic_bitset<>& GetRefsBitset() noexcept {
        return refs_;
    }
};

}  // namespace algos::spider
