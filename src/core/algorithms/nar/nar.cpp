#include "nar.h"

namespace model {
std::string NAR::ToString() const {
    std::ostringstream result;
    result << std::to_string(qualities_.fitness) << " {";
    for (auto it{ante_.begin()}; it != ante_.end(); ++it) {
        if (it != ante_.begin()) {
            result << ", ";
        }
        result << it->first << ": " << it->second->ToString();
    }
    result << "} ===> {";
    for (auto it{cons_.begin()}; it != cons_.end(); ++it) {
        if (it != cons_.begin()) {
            result << ", ";
        }
        result << it->first << ": " << it->second->ToString();
    }
    result << "} s: " << qualities_.support << " c: " << qualities_.confidence;
    return result.str();
}

NARQualities CalcQualities(size_t num_rows_fit_ante, size_t num_rows_fit_ante_and_cons,
                           size_t included_features, size_t feature_count, size_t num_rows) {
    if (num_rows_fit_ante == 0) {
        return {0.0, 0.0, 0.0};
    }
    double support = num_rows_fit_ante_and_cons / static_cast<double>(num_rows);
    if (support == 0.0) {
        return {0.0, 0.0, 0.0};
    }
    double confidence = num_rows_fit_ante_and_cons / static_cast<double>(num_rows_fit_ante);
    double inclusion = included_features / static_cast<double>(feature_count);
    double fitness = (confidence + support + inclusion) / 3.0;
    return {fitness, support, confidence};
}

void NAR::SetQualities(TypedRelation const* typed_relation) {
    if (ante_.size() == 0 || cons_.size() == 0) {
        qualities_ = {0.0, 0.0, 0.0};
        qualities_consistent_ = true;
        return;
    }
    size_t num_rows_fit_ante = 0;
    size_t num_rows_fit_ante_and_cons = 0;
    size_t num_rows = typed_relation->GetNumRows();
    size_t num_columns = typed_relation->GetNumColumns();
    for (size_t rowi = 0; rowi < typed_relation->GetNumRows(); ++rowi) {
        bool row_fits_ante = true;
        bool row_fits_cons = true;
        for (size_t coli = 0; coli < num_columns; ++coli) {
            model::TypedColumnData const& column = typed_relation->GetColumnData(coli);
            auto value = column.GetValue(rowi);
            if (row_fits_ante) {
                row_fits_ante = AnteFitsValue(coli, value);
                row_fits_cons &= ConsFitsValue(coli, value);
            } else {
                break;
            }
        }
        if (row_fits_ante) {
            ++num_rows_fit_ante;
            if (row_fits_cons) {
                ++num_rows_fit_ante_and_cons;
            }
        }
    }

    qualities_ = CalcQualities(num_rows_fit_ante, num_rows_fit_ante_and_cons,
                               ante_.size() + cons_.size(), num_columns, num_rows);
    qualities_consistent_ = true;
}

model::NARQualities const& NAR::GetQualities() const {
    if (!qualities_consistent_) {
        throw std::logic_error("Getting uninitialized qualities from NAR.");
    }
    return qualities_;
}

void NAR::InsertInAnte(size_t feature_index, std::shared_ptr<ValueRange> range) {
    qualities_consistent_ = false;
    ante_.insert({feature_index, std::move(range)});
}

void NAR::InsertInCons(size_t feature_index, std::shared_ptr<ValueRange> range) {
    qualities_consistent_ = false;
    cons_.insert({feature_index, std::move(range)});
}

bool NAR::MapFitsValue(std::map<size_t, std::shared_ptr<ValueRange>> const& map,
                       size_t feature_index, std::byte const* value) {
    if (!map.contains(feature_index)) {
        return true;
    }
    return map.at(feature_index)->Includes(value);
}

}  // namespace model
