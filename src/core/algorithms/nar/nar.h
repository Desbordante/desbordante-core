#pragma once

#include <vector>

#include "model/table/column_layout_typed_relation_data.h"
#include "model/types/types.h"
#include "value_range.h"

namespace model {

struct NARQualities {
    double fitness = -1.0;
    double support = -1.0;
    double confidence = -1.0;

    std::string ToString() const {
        std::ostringstream ss;
        ss << "fitness: " << fitness << " support: " << support << " confidence: " << confidence;
        return ss.str();
    }
};

class NAR {
private:
    using TypedRelation = model::ColumnLayoutTypedRelationData;
    NARQualities qualities_;
    bool qualities_consistent_ = false;

    std::unordered_map<size_t, std::shared_ptr<ValueRange>> ante_{};
    std::unordered_map<size_t, std::shared_ptr<ValueRange>> cons_{};

    bool AnteFitsValue(size_t feature_index, std::byte const* value_of_feature) const {
        return MapFitsValue(ante_, feature_index, value_of_feature);
    }

    bool ConsFitsValue(size_t feature_index, std::byte const* value_of_feature) const {
        return MapFitsValue(cons_, feature_index, value_of_feature);
    }

    static bool MapFitsValue(std::unordered_map<size_t, std::shared_ptr<ValueRange>> const& map,
                             size_t feature_index, std::byte const* value);

public:
    std::string ToString() const;
    void SetQualities(TypedRelation const* typed_relation);
    NARQualities const& GetQualities() const;

    auto const& GetAnte() const noexcept {
        return ante_;
    }

    auto const& GetCons() const noexcept {
        return cons_;
    }

    void InsertInAnte(size_t feature_index, std::shared_ptr<ValueRange> range);
    void InsertInCons(size_t feature_index, std::shared_ptr<ValueRange> range);
};

}  // namespace model
