#include "ac_exception_finder.h"

#include "ac_algorithm.h"
#include "bin_operation_enum.h"
#include "type_wrapper.h"

namespace algos::algebraic_constraints {

bool ACExceptionFinder::ValueBelongsToRanges(RangesCollection const& ranges_collection,
                                             std::byte const* val) {
    for (size_t i = 0; i < ranges_collection.ranges.size() - 1; i += 2) {
        std::byte const* l_border = ranges_collection.ranges[i];
        std::byte const* r_border = ranges_collection.ranges[i + 1];
        if (ranges_collection.col_pair.type_wrapper.NumericCompare(l_border, val) ==
                    model::CompareResult::kEqual ||
            ranges_collection.col_pair.type_wrapper.NumericCompare(val, r_border) ==
                    model::CompareResult::kEqual) {
            return true;
        }
        if (ranges_collection.col_pair.type_wrapper.NumericCompare(l_border, val) ==
                    model::CompareResult::kLess &&
            ranges_collection.col_pair.type_wrapper.NumericCompare(val, r_border) ==
                    model::CompareResult::kLess) {
            return true;
        }
    }
    return false;
}

void ACExceptionFinder::AddException(size_t row_i, std::pair<size_t, size_t> const& col_pair) {
    auto equal = [row_i](ACException const& e) { return e.row_i == row_i; };
    auto exception_i = std::find_if(exceptions_.begin(), exceptions_.end(), equal);
    if (exception_i == exceptions_.end()) {
        exceptions_.push_back(ACException(row_i, col_pair));
    } else {
        exception_i->column_pairs.push_back(col_pair);
    }
}

void ACExceptionFinder::CollectColumnPairExceptions(std::vector<model::TypedColumnData> const& data,
                                                    RangesCollection const& ranges_collection) {
    size_t lhs_i = ranges_collection.col_pair.col_i.first;
    size_t rhs_i = ranges_collection.col_pair.col_i.second;
    std::vector<std::byte const*> const& lhs = data.at(lhs_i).GetData();
    std::vector<std::byte const*> const& rhs = data.at(rhs_i).GetData();
    TypeWrapper type_wrapper(data.at(lhs_i).GetTypeId());
    for (size_t i = 0; i < lhs.size(); ++i) {
        std::byte const* l = lhs.at(i);
        std::byte const* r = rhs.at(i);
        if (data[lhs_i].IsNullOrEmpty(i) || data[rhs_i].IsNullOrEmpty(i)) {
            continue;
        }
        std::unique_ptr<std::byte[]> res =
                std::unique_ptr<std::byte[]>(type_wrapper.NumericAllocate());
        type_wrapper.NumericFromStr(res.get(), "0");

        if (ac_alg_->GetBinOperation() == +Binop::Division &&
            type_wrapper.NumericCompare(r, res.get()) == model::CompareResult::kEqual) {
            continue;
        }
        ac_alg_->InvokeBinop(l, r, res.get());
        if (!ValueBelongsToRanges(ranges_collection, res.get())) {
            AddException(i, {lhs_i, rhs_i});
        }
    }
}

void ACExceptionFinder::CollectExceptions(algos::ACAlgorithm const* ac_alg) {
    ac_alg_ = ac_alg;
    std::vector<model::TypedColumnData> const& data = ac_alg_->GetTypedData();
    std::vector<RangesCollection> const& ranges = ac_alg_->GetRangesCollections();
    for (auto const& ranges_collection : ranges) {
        CollectColumnPairExceptions(data, ranges_collection);
    }
    auto comp = [](ACException const& a, ACException const& b) { return a.row_i < b.row_i; };
    std::sort(exceptions_.begin(), exceptions_.end(), comp);
}

}  // namespace algos::algebraic_constraints
