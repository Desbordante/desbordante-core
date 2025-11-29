#pragma once

#include <memory>

#include "core/model/table/vertical.h"

namespace model {

// UCC stands for Unique Column Colmbinations and denotes a list of columns whose projection
// contains no duplicate entry. I.e. there is no tuples that have equal values in attributes
// presenting in the UCC.
// Defining UCC as Vertical, because we don't really need more functionality than Vertical provides.
class UCC : public Vertical {
private:
    std::shared_ptr<RelationalSchema const> schema_;

public:
    UCC(std::shared_ptr<RelationalSchema const> schema, boost::dynamic_bitset<> indices)
        : Vertical(schema.get(), std::move(indices)), schema_(std::move(schema)) {}

    UCC(std::shared_ptr<RelationalSchema const> schema, Vertical ucc_val)
        : Vertical(std::move(ucc_val)), schema_(std::move(schema)) {}

    UCC() = default;

    std::shared_ptr<RelationalSchema const> GetSchema() const {
        return schema_;
    }
};

}  // namespace model
