#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <frozen/unordered_map.h>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dc/model/dc.h"
#include "core/algorithms/dc/model/point.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/table/typed_column_data.h"
#include "core/util/kdtree.h"

namespace algos {

namespace dc {

using Violation = std::pair<size_t, size_t>;

struct SetComparator {
    bool operator()(Violation const& lhs, Violation const& rhs) const {
        return (lhs.first != rhs.first) ? (lhs.first < rhs.first) : (lhs.second < rhs.second);
    }
};

}  // namespace dc

class DCVerifier final : public Algorithm {
private:
    // @brief Represents violating tuples of a given table
    // e.g. vector {{1, 4}, {1, 3}} tells us that records with
    // number 1 and 4, 1 and 3 are violating the given Denial Constraint.
    // Thus it is possible to remove first record so the DC holds.
    //
    // If a certain pair contains equal left and right record number
    // it means that that DC is a one-tuple one and it sufficient
    // for a single tuple to violate it. e.g. {{2, 2}}
    std::set<dc::Violation, dc::SetComparator> violations_;
    std::unique_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::TypedColumnData> data_;
    size_t index_offset_;
    bool result_;
    dc::DC dc_;

    // options
    config::InputTable input_table_;
    std::string dc_string_;
    bool do_collect_violations_;
    bool enable_ordering_;

    void RegisterOptions();

    void MakeExecuteOptsAvailable();

    bool Verify(std::string dc);

    bool VerifyOneTuple(dc::DC const& dc);

    bool VerifyTwoTuples(dc::DC const& dc);

    bool VerifyMixed(dc::DC const& dc);

    bool VerifyOneInequality(dc::DC const& dc);

    bool VerifyAllEquality(dc::DC const& dc);

    std::vector<std::byte const*> GetRow(size_t row) const;

    bool Eval(std::vector<std::byte const*> tuple, std::vector<dc::Predicate> preds) const;

    bool ContainsNullOrEmpty(std::vector<Column::IndexType> const& indices, size_t tuple_ind) const;

    std::pair<util::Rect<dc::Point<dc::Component>>, util::Rect<dc::Point<dc::Component>>>
    SearchRanges(std::vector<Column::IndexType> const& all_cols, dc::DC const& ineq_dc,
                 std::vector<std::byte const*> const& tuple) const;

    util::Rect<dc::Point<dc::Component>> SearchMixedRange(dc::DC const& dc, dc::DC const& mixed_dc,
                                                          std::vector<std::byte const*> row);

    std::optional<dc::Point<dc::Component>> ProcessMixed(
            util::KDTree<dc::Point<dc::Component>> const& search_tree,
            std::vector<dc::Predicate> const& const_preds, dc::DC const& two_tuple_dc,
            std::vector<Column::IndexType> const& all_cols, size_t i,
            std::unordered_set<dc::Point<dc::Component>, dc::Point<dc::Component>::Hasher>&
                    res_points);

    dc::Point<dc::Component> MakePoint(std::vector<std::byte const*> const& vec,
                                       std::vector<Column::IndexType> const& indices,
                                       size_t point_ind = 0,
                                       dc::ValType val_type = dc::ValType::kFinite) const;

    dc::DC GetDC(std::vector<dc::Predicate> const& no_diseq_preds,
                 std::vector<dc::Predicate> const& diseq_preds, size_t cur_signs);

    static constexpr frozen::unordered_map<dc::DCType, bool (DCVerifier::*)(dc::DC const&), 5>
            kDCTypeToVerificationMethod{
                    {dc::DCType::kAllEquality, &DCVerifier::VerifyAllEquality},
                    {dc::DCType::kOneInequality, &DCVerifier::VerifyOneInequality},
                    {dc::DCType::kOneTuple, &DCVerifier::VerifyOneTuple},
                    {dc::DCType::kTwoTuples, &DCVerifier::VerifyTwoTuples},
                    {dc::DCType::kMixed, &DCVerifier::VerifyMixed}};

    template <typename InputIt>
    void AddHighlights(InputIt first, InputIt last, size_t cur_index) {
        for (; first != last; ++first) {
            violations_.emplace(first->GetIndex(), cur_index);
        }
    }

    void EnrichViolations();

    bool ViolationHolds(size_t s_ind, size_t t_ind, dc::DC const& dc);

public:
    DCVerifier();

    bool DCHolds() const noexcept {
        return result_;
    }

    std::vector<dc::Violation> GetViolations() {
        if (enable_ordering_) EnrichViolations();
        return {violations_.begin(), violations_.end()};
    }

    void ResetState() final {
        violations_.clear();
    };

    size_t GetIndexOffset() const noexcept {
        assert(relation_);
        std::string col_name = relation_->GetSchema()->GetColumns().front().get()->GetName();
        boost::regex re("[0-9]+");
        bool has_header = !boost::regex_match(col_name, re);
        return 1 + static_cast<size_t>(has_header);
    }

    void LoadDataInternal() final;
    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos
