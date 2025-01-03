#include "nar.h"

namespace model {

std::string NAR::ToString() const {
    auto map_to_string = [](auto const& map) {
        std::ostringstream os;
        bool first = true;
        for (auto const& [key, range] : map) {
            if (!first) {
                os << ", ";
            }
            os << key << ": " << range->ToString();
            first = false;
        }
        return os.str();
    };

    std::ostringstream result;
    result << qualities_.fitness << " {";
    result << map_to_string(ante_) << "} ===> {" << map_to_string(cons_);
    result << "} s: " << qualities_.support << " c: " << qualities_.confidence;
    return result.str();
}

NARQualities CalcQualities(size_t num_rows_fit_ante, size_t num_rows_fit_ante_and_cons,
                           size_t included_features, size_t feature_count, size_t num_rows) {
    if (num_rows_fit_ante == 0) {
        return {0.0, 0.0, 0.0};
    }
    double support = static_cast<double>(num_rows_fit_ante_and_cons) / num_rows;
    if (support == 0.0) {
        return {0.0, 0.0, 0.0};
    }
    double confidence = static_cast<double>(num_rows_fit_ante_and_cons) / num_rows_fit_ante;
    double inclusion = static_cast<double>(included_features) / feature_count;
    double fitness = (confidence + support + inclusion) / 3.0;
    return {fitness, support, confidence};
}

void NAR::SetQualities(TypedRelation const* typed_relation) {
    if (ante_.empty() || cons_.empty()) {
        qualities_ = {0.0, 0.0, 0.0};
        qualities_consistent_ = true;
        return;
    }
    size_t num_rows_fit_ante = 0;
    size_t num_rows_fit_ante_and_cons = 0;
    size_t num_rows = typed_relation->GetNumRows();
    size_t num_columns = typed_relation->GetNumColumns();
    for (size_t rowi = 0; rowi < num_rows; ++rowi) {
        bool row_fits_ante = true;
        bool row_fits_cons = true;
        for (size_t coli = 0; coli < num_columns; ++coli) {
            model::TypedColumnData const& column = typed_relation->GetColumnData(coli);
            std::byte const* value = column.GetValue(rowi);
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

bool NAR::MapFitsValue(std::unordered_map<size_t, std::shared_ptr<ValueRange>> const& map,
                       size_t feature_index, std::byte const* value) {
    auto it = map.find(feature_index);
    if (it == map.end()) {
        return true;
    }
    return it->second->Includes(value);
}

}  // namespace model
