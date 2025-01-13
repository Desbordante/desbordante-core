#include "algorithms/dc/verifier/dc_verifier.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <easylogging++.h>

#include "algorithms/dc/model/component.h"
#include "algorithms/dc/model/point.h"
#include "algorithms/dc/model/predicate.h"
#include "algorithms/dc/parser/dc_parser.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_relation_data.h"
#include "table/typed_column_data.h"
#include "util/get_preallocated_vector.h"
#include "util/kdtree.h"

namespace algos {

namespace mo = model;

using Point = dc::Point<dc::Component>;
using Tree = util::KDTree<Point>;

DCVerifier::DCVerifier() : Algorithm({}) {
    using namespace config::names;

    RegisterOptions();
    MakeOptionsAvailable({kTable});
};

void DCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<std::string>(&dc_string_, kDenialConstraint, kDDenialConstraint, ""));
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<bool>(&do_collect_violations_, kDoCollectViolations,
                                kDDoCollectViolations, false));
}

void DCVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kDenialConstraint});
    MakeOptionsAvailable({config::names::kDoCollectViolations});
}

void DCVerifier::LoadDataInternal() {
    data_ = model::CreateTypedColumnData(*input_table_, true);
    input_table_->Reset();
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, true);
}

unsigned long long int DCVerifier::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    dc::DC dc;
    try {
        dc::DCParser parser = dc::DCParser(dc_string_, relation_.get(), data_);
        dc = parser.Parse();
    } catch (std::exception const& e) {
        LOG(INFO) << e.what();
        return 0;
    }

    std::string col_name = relation_->GetSchema()->GetColumns().front().get()->GetName();
    boost::regex re("[0-9]+");
    bool has_header = !boost::regex_match(col_name, re);
    index_offset_ = 1 + static_cast<size_t>(has_header);
    result_ = Verify(dc);

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);

    return elapsed_time.count();
}

bool DCVerifier::Verify(dc::DC dc) {
    dc.ConvertEqualities();
    std::vector<dc::Predicate> no_diseq_preds, diseq_preds;
    std::vector<dc::Predicate> predicates = dc.GetPredicates();

    for (auto& pred : predicates) {
        auto op = pred.GetOperator();
        if (pred.IsVariable() and pred.IsCrossTuple() and
            op.GetType() == dc::OperatorType::kUnequal) {
            diseq_preds.emplace_back(pred);
        } else {
            no_diseq_preds.emplace_back(pred);
        }
    }

    size_t diseq_preds_count = diseq_preds.size();
    size_t all_comb_count = std::pow(2, diseq_preds_count);
    dc::DCType dc_type = dc.GetType();

    // Otherwise it is not possible to collect violations
    if (dc_type == dc::DCType::kOneInequality and do_collect_violations_ == true)
        dc_type = dc::DCType::kTwoTuples;

    auto check = kDCTypeToVerificationMethod.at(dc_type);

    // TODO: check the article for optimization 2^l -> 2^(l-1) dc's

    // Consider 'cur_signs' as a set of operators where each digit
    // in binary representation (0 or 1) means '<' or '>' respectively
    // e.g. 21 = 10101 is ">, <, >, <, >"
    bool res = true;
    for (size_t cur_signs = 0; cur_signs < all_comb_count; ++cur_signs) {
        dc::DC cur_dc = GetDC(no_diseq_preds, diseq_preds, cur_signs);
        res = (this->*check)(cur_dc);
        if (!do_collect_violations_ and !res) return false;
    }

    return violations_.empty();
}

dc::DC DCVerifier::GetDC(std::vector<dc::Predicate> const& no_diseq_preds,
                         std::vector<dc::Predicate> const& diseq_preds, size_t cur_signs) {
    size_t diseq_preds_count = diseq_preds.size();
    constexpr static dc::OperatorType kBitOperatorTypeMap[] = {dc::OperatorType::kLess,
                                                               dc::OperatorType::kGreater};
    std::vector<dc::Predicate> cur_predicates =
            util::GetPreallocatedVector<dc::Predicate>(no_diseq_preds.size() + diseq_preds_count);
    cur_predicates.insert(cur_predicates.begin(), no_diseq_preds.begin(), no_diseq_preds.end());
    for (size_t i = 0; i < diseq_preds_count; ++i) {
        bool cur_sign = cur_signs & (1 << i);
        dc::OperatorType cur_operator_type = kBitOperatorTypeMap[cur_sign];
        dc::Predicate const& diseq_pred = diseq_preds[i];
        auto left_op = diseq_pred.GetLeftOperand();
        auto right_op = diseq_pred.GetRightOperand();
        cur_predicates.emplace_back(cur_operator_type, left_op, right_op);
    }

    return {std::move(cur_predicates)};
}

bool DCVerifier::VerifyMixed(dc::DC const& dc) {
    std::vector<dc::Predicate> predicates = dc.GetPredicates();
    std::vector<dc::Predicate> s_predicates, t_predicates, mixed_predicates;
    for (auto pred : predicates) {
        bool left_op_from_first_tuple = pred.GetLeftOperand().IsFirstTuple();
        bool right_op_from_first_tuple = pred.GetRightOperand().IsFirstTuple();
        switch (left_op_from_first_tuple + right_op_from_first_tuple) {
            case 0:  // s.A op s.B
                s_predicates.emplace_back(pred);
                break;
            case 1:  // s.A op t.B or t.A op s.B
                mixed_predicates.emplace_back(pred);
                break;
            case 2:  // t.A op t.B
                t_predicates.emplace_back(pred);
                break;
            default:
                assert(false);
                __builtin_unreachable();
        }
    }

    util::KDTree<Point> s_tree, t_tree;
    std::vector<mo::ColumnIndex> all_cols = dc.GetColumnIndices();
    std::vector<dc::Predicate> var_preds =
            dc.GetPredicates([](dc::Predicate const& pred) { return pred.IsVariable(); });

    for (size_t i = 0; i < data_.front().GetNumRows(); ++i) {
        std::unordered_set<Point, Point::Hasher> res_points;
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        auto process =
                std::bind(&DCVerifier::ProcessMixed, this, std::placeholders::_1,
                          std::placeholders::_2, var_preds, all_cols, i, std::ref(res_points));

        std::optional<Point> t_point = process(s_tree, t_predicates);
        std::optional<Point> s_point = process(t_tree, s_predicates);

        if (do_collect_violations_ == true) {
            AddHighlights(res_points.begin(), res_points.end(), i + index_offset_);
        } else if (!res_points.empty()) {
            return false;
        }

        if (t_point.has_value()) t_tree.Insert(t_point.value());
        if (s_point.has_value()) s_tree.Insert(s_point.value());
    }

    return violations_.empty();
}

std::optional<Point> DCVerifier::ProcessMixed(
        Tree const& search_tree, std::vector<dc::Predicate> const& const_preds,
        dc::DC const& two_tuple_dc, std::vector<mo::ColumnIndex> const& all_cols, size_t i,
        std::unordered_set<Point, Point::Hasher>& res_points) {
    std::vector<std::byte const*> row = GetRow(i);
    auto [box, inv_box] = SearchRanges(all_cols, two_tuple_dc, row);

    std::optional<Point> insert_point;
    if (Eval(row, const_preds)) {
        std::vector<Point> search_res = search_tree.QuerySearch(box);
        res_points.insert(search_res.begin(), search_res.end());

        size_t cur_ind = i + index_offset_;
        insert_point = MakePoint(row, all_cols, cur_ind);
    }
    return insert_point;
}

bool DCVerifier::VerifyOneTuple(dc::DC const& dc) {
    std::vector<Column::IndexType> all_cols = dc.GetColumnIndices();
    for (size_t i = 0; i < data_.front().GetNumRows(); ++i) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetRow(i);
        if (Eval(tuple, dc.GetPredicates())) {
            size_t cur_ind = i + index_offset_;

            if (do_collect_violations_) {
                violations_.emplace(cur_ind, cur_ind);
            } else {
                return false;
            }
        }
    }

    return violations_.empty();
}

bool DCVerifier::VerifyTwoTuples(dc::DC const& dc) {
    std::vector<mo::ColumnIndex> all_cols = dc.GetColumnIndices();
    std::vector<mo::ColumnIndex> eq_cols = dc.GetColumnIndicesWithOperator(
            [](dc::Operator op) { return op.GetType() == dc::OperatorType::kEqual; });
    std::vector<mo::ColumnIndex> ineq_cols = dc.GetColumnIndicesWithOperator(
            [](dc::Operator op) { return op.GetType() != dc::OperatorType::kEqual; });

    std::vector<Point> points;
    std::unordered_map<Point, util::KDTree<Point>, Point::Hasher> hash;
    std::vector<dc::Predicate> ineq_preds = dc.GetPredicates([](dc::Predicate pred) {
        return pred.GetOperator().GetType() != dc::OperatorType::kEqual;
    });

    for (size_t i = 0; i < data_.front().GetNumRows(); ++i) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;

        std::vector<std::byte const*> row = GetRow(i);
        Point point = MakePoint(row, eq_cols);
        Tree& tree = hash[point];
        auto [box, inv_box] = SearchRanges(ineq_cols, ineq_preds, row);

        std::unordered_set<Point, Point::Hasher> res_points;
        std::vector<Point> search_res = tree.QuerySearch(box);
        std::vector<Point> inv_search_res = tree.QuerySearch(inv_box);

        res_points.insert(search_res.begin(), search_res.end());
        res_points.insert(inv_search_res.begin(), inv_search_res.end());

        size_t cur_ind = i + index_offset_;
        if (do_collect_violations_ == true) {
            AddHighlights(res_points.begin(), res_points.end(), cur_ind);
        } else if (!res_points.empty()) {
            return false;
        }

        tree.Insert(MakePoint(row, ineq_cols, cur_ind));
    }

    return violations_.empty();
}

bool DCVerifier::VerifyAllEquality(dc::DC const& dc) {
    std::unordered_map<Point, std::vector<size_t>, Point::Hasher> res_tuples;
    std::vector<Column::IndexType> const eq_cols = dc.GetColumnIndices();
    for (size_t i = 0; i < data_.front().GetNumRows(); ++i) {
        if (ContainsNullOrEmpty(eq_cols, i)) continue;

        std::vector<std::byte const*> row = GetRow(i);
        size_t cur_ind = i + index_offset_;
        Point point = MakePoint(row, eq_cols);

        if (res_tuples.find(point) != res_tuples.end()) {
            if (!do_collect_violations_) return false;

            std::vector<size_t> viol_indexes = res_tuples[point];
            for (auto ind : viol_indexes) {
                violations_.emplace(cur_ind, ind);
            }
            res_tuples[point].push_back(cur_ind);
        } else {
            res_tuples[point] = {cur_ind};
        }
    }

    return violations_.empty();
}

bool DCVerifier::VerifyOneInequality(dc::DC const& dc) {
    std::vector<dc::Predicate> predicates = dc.GetPredicates();
    dc::Predicate ineq_pred =
            dc.GetPredicates([](dc::Predicate const& pred) {
                  return pred.GetOperator().GetType() != dc::OperatorType::kEqual and
                         pred.GetOperator().GetType() != dc::OperatorType::kUnequal;
              }).front();

    mo::ColumnIndex ind_a = ineq_pred.GetLeftOperand().GetColumn()->GetIndex();
    mo::ColumnIndex ind_b = ineq_pred.GetRightOperand().GetColumn()->GetIndex();
    mo::Type const& type_a = data_[ind_a].GetType();
    mo::Type const& type_b = data_[ind_b].GetType();

    std::vector<mo::ColumnIndex> const eq_cols = dc.GetColumnIndicesWithOperator(
            [](dc::Operator op) { return op.GetType() == dc::OperatorType::kEqual; });
    std::unordered_map<Point, dc::Component, Point::Hasher> min_a, min_b, max_a, max_b;
    std::vector<mo::ColumnIndex> all_cols = dc.GetColumnIndices();

    for (size_t i = 0; i < data_.front().GetNumRows(); ++i) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;

        auto min_comp = dc::Component(nullptr, &type_a, dc::ValType::kPlusInf);
        auto max_comp = dc::Component(nullptr, &type_b, dc::ValType::kMinusInf);

        std::vector<std::byte const*> row = GetRow(i);
        Point point = MakePoint(row, eq_cols);

        dc::Component const& min_a_element = min_a.try_emplace(point, min_comp).first->second;
        dc::Component const& min_b_element = min_b.try_emplace(point, min_comp).first->second;
        dc::Component const& max_a_element = max_a.try_emplace(point, max_comp).first->second;
        dc::Component const& max_b_element = max_b.try_emplace(point, max_comp).first->second;

        auto left_comp = dc::Component(row[ind_a], &type_a);
        auto right_comp = dc::Component(row[ind_b], &type_b);

        dc::Operator op = ineq_pred.GetOperator();
        dc::OperatorType op_type = op.GetType();

        if (op_type == dc::OperatorType::kLess or op_type == dc::OperatorType::kLessEqual) {
            if (dc::Component::Eval(min_a[point], right_comp, op) or
                dc::Component::Eval(left_comp, max_b[point], op))
                return false;
        } else if (dc::Component::Eval(max_a[point], right_comp, op) or
                   dc::Component::Eval(left_comp, min_b[point], op)) {
            return false;
        }

        min_a[point] = std::min(min_a_element, left_comp);
        min_b[point] = std::min(min_b_element, right_comp);
        max_a[point] = std::max(max_a_element, left_comp);
        max_b[point] = std::max(max_b_element, right_comp);
    }

    return true;
}

std::vector<std::byte const*> DCVerifier::GetRow(size_t row) const {
    auto res = std::vector<std::byte const*>(data_.size());
    auto get_val = [row](auto const& col) { return col.GetValue(row); };
    std::transform(data_.begin(), data_.end(), res.begin(), get_val);

    return res;
}

std::pair<util::Rect<Point>, util::Rect<Point>> DCVerifier::SearchRanges(
        std::vector<mo::ColumnIndex> const& cols_to_select, dc::DC const& ineq_dc,
        std::vector<std::byte const*> const& row) const {
    std::vector<mo::ColumnIndex> ineq_cols = ineq_dc.GetColumnIndices();
    std::sort(ineq_cols.begin(), ineq_cols.end());

    std::vector<std::byte const*> empty_vec(row.size(), nullptr);
    Point lower_bound = MakePoint(empty_vec, cols_to_select, 0, dc::ValType::kMinusInf);
    Point upper_bound = MakePoint(empty_vec, cols_to_select, 0, dc::ValType::kPlusInf);
    Point lower_bound_inv = lower_bound, upper_bound_inv = upper_bound;

    std::vector<dc::Predicate> ineq_preds = ineq_dc.GetPredicates();

    using bound_type = util::Rect<Point>::bound_type;

    std::vector<bound_type> lower_bound_type(cols_to_select.size(), bound_type::kClosed);
    std::vector<bound_type> upper_bound_type(cols_to_select.size(), bound_type::kClosed);
    std::vector<bound_type> lower_bound_type_inv(cols_to_select.size(), bound_type::kClosed);
    std::vector<bound_type> upper_bound_type_inv(cols_to_select.size(), bound_type::kClosed);

    for (dc::Predicate const& ineq_pred : ineq_preds) {
        dc::OperatorType op_type = ineq_pred.GetOperator().GetType();
        mo::ColumnIndex left = ineq_pred.GetLeftOperand().GetColumn()->GetIndex();
        mo::ColumnIndex right = ineq_pred.GetRightOperand().GetColumn()->GetIndex();
        mo::ColumnIndex left_ind = 0, right_ind = 0;

        while (cols_to_select[left_ind] != left) left_ind++;
        while (cols_to_select[right_ind] != right) right_ind++;

        auto right_comp = dc::Component(row[right], &data_[right].GetType());
        auto left_comp = dc::Component(row[left], &data_[left].GetType());

        if (op_type == dc::OperatorType::kLessEqual or op_type == dc::OperatorType::kLess) {
            upper_bound[left_ind] = std::min(upper_bound[left_ind], right_comp);
            lower_bound_inv[right_ind] = std::max(lower_bound_inv[right_ind], left_comp);

            if (op_type == dc::OperatorType::kLess) {
                upper_bound_type[left_ind] = bound_type::kOpen;
                lower_bound_type_inv[right_ind] = bound_type::kOpen;
            }
        } else if (op_type == dc::OperatorType::kGreaterEqual or
                   op_type == dc::OperatorType::kGreater) {
            lower_bound[left_ind] = std::max(lower_bound[left_ind], right_comp);
            upper_bound_inv[right_ind] = std::min(upper_bound_inv[right_ind], left_comp);

            if (op_type == dc::OperatorType::kGreater) {
                lower_bound_type[left_ind] = bound_type::kOpen;
                upper_bound_type_inv[right_ind] = bound_type::kOpen;
            }
        }
    }

    util::Rect<Point> box(lower_bound, upper_bound);
    util::Rect<Point> inv_box(lower_bound_inv, upper_bound_inv);

    box.SetBoundType(lower_bound_type, upper_bound_type);
    inv_box.SetBoundType(lower_bound_type_inv, upper_bound_type_inv);

    return {box, inv_box};
}

bool DCVerifier::Eval(std::vector<std::byte const*> row, std::vector<dc::Predicate> preds) const {
    dc::Component left_comp, right_comp;
    std::byte const* left_val;
    std::byte const* right_val;
    for (auto const& pred : preds) {
        dc::ColumnOperand left_op = pred.GetLeftOperand();
        dc::ColumnOperand right_op = pred.GetRightOperand();

        if (left_op.IsConstant()) left_val = left_op.GetVal();
        if (right_op.IsConstant()) right_val = right_op.GetVal();

        if (left_op.IsVariable()) left_val = row[left_op.GetColumn()->GetIndex()];
        if (right_op.IsVariable()) right_val = row[right_op.GetColumn()->GetIndex()];

        left_comp = dc::Component(left_val, left_op.GetType());
        right_comp = dc::Component(right_val, right_op.GetType());

        if (!dc::Component::Eval(left_comp, right_comp, pred.GetOperator())) return false;
    }

    return true;
}

Point DCVerifier::MakePoint(std::vector<std::byte const*> const& vec,
                            std::vector<mo::ColumnIndex> const& indices, size_t point_ind /* = 0 */,
                            dc::ValType val_type /* = kFinite */) const {
    std::vector<dc::Component> pt;
    for (auto ind : indices) {
        mo::Type const& type = data_[ind].GetType();
        pt.emplace_back(vec[ind], &type, val_type);
    }

    return {std::move(pt), point_ind};
}

bool DCVerifier::ContainsNullOrEmpty(std::vector<mo::ColumnIndex> const& indices,
                                     size_t tuple_ind) const {
    auto l = [this, tuple_ind](mo::ColumnIndex ind) { return data_[ind].IsNullOrEmpty(tuple_ind); };
    return std::any_of(indices.begin(), indices.end(), l);
}

}  // namespace algos
