#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "pac/model/metrizable_tuple.h"
#include "type.h"

namespace algos::pac_verifier {
// TODO(senichenkov): maybe interface IPACHighlight?
// Upd: Looks like there's no Domain PAC-specific things, maybe this class may become PACHighlight?
class DomainPACHighlight {
private:
    using Tuples = std::vector<pac::model::Tuple>;
    using TuplesIter = Tuples::iterator;

    std::shared_ptr<pac::model::MetrizableTupleType> tuple_type_;
    std::shared_ptr<Tuples> original_value_tuples_;
    std::vector<TuplesIter> highlighted_tuples_;

public:
    DomainPACHighlight(std::shared_ptr<pac::model::MetrizableTupleType> tuple_type,
                       std::shared_ptr<Tuples> original_value_tuples,
                       std::vector<TuplesIter>&& highlighted_tuples)
        : tuple_type_(std::move(tuple_type)),
          original_value_tuples_(std::move(original_value_tuples)),
          highlighted_tuples_(std::move(highlighted_tuples)) {}

    /// @brief Get row numbers of highlighted values
    std::vector<std::size_t> GetIndices() const {
        std::vector<std::size_t> indices;
        for (auto const iter : highlighted_tuples_) {
            indices.push_back(std::distance(original_value_tuples_->begin(), iter));
        }
        return indices;
    }

    /// @brief Get @c Types of columns associated with this @c Highlight
    std::vector<model::Type const*> const& GetTypes() const {
        return tuple_type_->GetTypes();
    }

    /// @brief Get highlighted values as pointers to @c std::byte, that can be used with types (see
    /// @c GetTypes())
    Tuples GetByteData() const {
        Tuples tuples;
        for (auto const it : highlighted_tuples_) {
            tuples.push_back(*it);
        }
        return tuples;
    }

    /// @brief Get highlighted values as strings
    std::vector<std::string> GetStringData() const {
        std::vector<std::string> strings;
        for (auto const it : highlighted_tuples_) {
            strings.push_back(tuple_type_->ValueToString(*it));
        }
        return strings;
    }
};
}  // namespace algos::pac_verifier
