#include "algorithms/md/hymd/preprocessing/similarity_measure/date_difference.h"

size_t DateDifference(model::Date const& left, model::Date const& right) {
    model::DateType date_type;
    std::byte* left_buf = reinterpret_cast<std::byte*>(new model::Date(left));
    std::byte* right_buf = reinterpret_cast<std::byte*>(new model::Date(right));

    auto diff = date_type.SubDate(left_buf, right_buf);

    delete reinterpret_cast<model::Date*>(left_buf);
    delete reinterpret_cast<model::Date*>(right_buf);

    return std::abs(diff.days());
}