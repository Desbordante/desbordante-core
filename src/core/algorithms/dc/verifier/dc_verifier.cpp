#include "dc_verifier.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/functional/hash/hash.hpp>
#include <easylogging++.h>

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "dc/model/component.h"
#include "dc/model/point.h"
#include "dc/model/predicate.h"
#include "frozen/string.h"
#include "model/table/column_layout_relation_data.h"
#include "util/kdtree.h"

namespace algos {

using namespace dc;

namespace mo = model;

using point = Point<Component>;

DCVerifier::DCVerifier() : Algorithm({}) {
    using namespace config::names;

    RegisterOptions();
    MakeOptionsAvailable({kTable});
};

void DCVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<std::string>(&dc_string_, kDenialConstraint, kDDenialConstraint, ""));
    RegisterOption(config::kTableOpt(&input_table_));
}

void DCVerifier::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kDenialConstraint});
}

void DCVerifier::LoadDataInternal() {
    data_ = model::CreateTypedColumnData(*input_table_, true);
    input_table_->Reset();
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, true);
}

unsigned long long int DCVerifier::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    DC dc;
    try {
        dc = SplitDC(dc_string_);
    } catch (std::exception const& e) {
        LOG(INFO) << e.what();
        return 0;
    }

    dc = ConvertEqualities(dc);
    std::vector<DC> res_dc = ConvertDisequalities(dc);
    DCType dc_type = GetType(res_dc[0]);
    auto check = [&res_dc](DCVerifier* ver, bool (DCVerifier::*mtd)(const DC&)) {
        return std::all_of(res_dc.begin(), res_dc.end(),
                           [&ver, &mtd](const DC& dc) { return (ver->*mtd)(dc); });
    };

    // FIX: For better perfomance methods that use range search
    // should better use bst in case of one-dimensional equality part
    std::unordered_map<DCType, bool (DCVerifier::*)(const DC&)> map{
            {DCType::kAllEquality, &DCVerifier::VerifyAllEquality},
            {DCType::kOneInequality, &DCVerifier::VerifyOneInequality},
            {DCType::kOneTuple, &DCVerifier::VerifyOneTuple},
            {DCType::kTwoTuples, &DCVerifier::VerifyTwoTuples},
            {DCType::kMixed, &DCVerifier::VerifyMixed}};
    result_ = check(this, map.at(dc_type));

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);
    LOG(DEBUG) << "";

    return elapsed_time.count();
}

bool DCVerifier::VerifyMixed(const DC& dc) {
    std::vector<Predicate> predicates = dc.GetPredicates();
    std::vector<Predicate> s_predicates, t_predicates, mixed_predicates;
    for (auto const& pred : predicates) {
        bool l_tuple = pred.GetLeftOperand().GetTuple();
        bool r_tuple = pred.GetRightOperand().GetTuple();
        switch (l_tuple + r_tuple) {
            case 0:  // s.A op s.B
                s_predicates.push_back(pred);
                break;
            case 1:  // s.A op t.B
                mixed_predicates.push_back(pred);
                break;
            case 2:  // t.A op t.B
                t_predicates.push_back(pred);
                break;
            default:
                assert(false);
                __builtin_unreachable();
        }
    }

    DC mixed_dc(mixed_predicates.begin(), mixed_predicates.end());
    util::KDTree<point> s_tree, t_tree;
    std::vector<uint> all_cols = dc.GetColumnIndices();
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetTuple(i);

        auto [t_box, t_inv_box] = SearchRanges(mixed_dc, tuple);
        if (Eval(tuple, s_predicates)) {
            if (!t_tree.QuerySearch(t_box).empty() or !t_tree.QuerySearch(t_inv_box).empty())
                return false;
            s_tree.Insert(MakePoint(tuple, all_cols));
        }

        auto [s_box, s_inv_box] = SearchRanges(mixed_dc, tuple);
        if (Eval(tuple, t_predicates)) {
            if (!s_tree.QuerySearch(s_box).empty() or !s_tree.QuerySearch(s_inv_box).empty())
                return false;
            t_tree.Insert(MakePoint(tuple, all_cols));
        }
    }

    return true;
}

bool DCVerifier::VerifyOneTuple(const DC& dc) {
    std::vector<uint> all_cols = dc.GetColumnIndices();
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetTuple(i);
        if (Eval(tuple, dc.GetPredicates())) return false;
    }

    return true;
}

bool DCVerifier::VerifyTwoTuples(const DC& dc) {
    std::vector<uint> all_cols = dc.GetColumnIndices();
    std::vector<uint> eq_cols = dc.GetColumnIndicesWithOperator(
            [](Operator op) { return op.GetType() == OperatorType::kEqual; });
    std::vector<uint> ineq_cols = dc.GetColumnIndicesWithOperator(
            [](Operator op) { return op.GetType() != OperatorType::kEqual; });

    std::vector<point> points;
    std::unordered_map<uint, util::KDTree<point>> hash;
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);

        if (hash.find(key) == hash.end()) hash[key] = util::KDTree<point>();

        auto [box, inv_box] = SearchRanges(dc, tuple);

        if (!hash[key].QuerySearch(box).empty() or !hash[key].QuerySearch(inv_box).empty())
            return false;

        hash[key].Insert(MakePoint(tuple, ineq_cols));
    }

    return true;
}

bool DCVerifier::VerifyAllEquality(const DC& dc) {
    std::vector<uint> const eq_cols = dc.GetColumnIndices();
    std::unordered_set<uint> res;
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        if (ContainsNullOrEmpty(eq_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);
        if (res.find(key) != res.end()) return false;
        res.insert(key);
    }

    return true;
}

bool DCVerifier::VerifyOneInequality(const DC& dc) {
    std::vector<Predicate> predicates = dc.GetPredicates();
    Predicate ineq_pred = dc.GetPredicates([](Predicate const& pred) {
        return pred.GetOperator().GetType() != OperatorType::kEqual and
               pred.GetOperator().GetType() != OperatorType::kUnequal;
    })[0];

    uint ind_a = ineq_pred.GetLeftOperand().GetColumn()->GetIndex();
    uint ind_b = ineq_pred.GetRightOperand().GetColumn()->GetIndex();
    mo::Type const& type_a = data_[ind_a].GetType();
    mo::Type const& type_b = data_[ind_b].GetType();

    std::vector<uint> const eq_cols = dc.GetColumnIndicesWithOperator(
            [](Operator op) { return op.GetType() == OperatorType::kEqual; });
    std::unordered_map<uint, Component> min_a, min_b, max_a, max_b;
    std::vector<uint> all_cols = dc.GetColumnIndices();

    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        if (ContainsNullOrEmpty(all_cols, i)) continue;
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);

        if (min_a.find(key) == min_a.end()) {
            min_a[key] = min_b[key] = Component(nullptr, &type_a, ValType::kPlusInf);
            max_a[key] = max_b[key] = Component(nullptr, &type_b, ValType::kMinusInf);
        }

        auto left_comp = Component(tuple[ind_a], &type_a);
        auto right_comp = Component(tuple[ind_b], &type_b);
        Operator op = ineq_pred.GetOperator();
        OperatorType op_type = op.GetType();

        if (op_type == OperatorType::kLess or op_type == OperatorType::kLessEqual) {
            if (Component::Eval(min_a[key], right_comp, op) or
                Component::Eval(left_comp, max_b[key], op))
                return false;
        } else if (Component::Eval(max_a[key], right_comp, op) or
                   Component::Eval(left_comp, min_b[key], op))
            return false;

        min_a[key] = std::min(min_a[key], left_comp);
        min_b[key] = std::min(min_b[key], right_comp);
        max_a[key] = std::max(max_a[key], left_comp);
        max_b[key] = std::max(max_b[key], right_comp);
    }

    return true;
}

bool DCVerifier::CheckAllEquality(const DC& dc) {
    std::vector<Predicate> predicates = dc.GetPredicates();

    for (Predicate const& pred : predicates) {
        Operator op = pred.GetOperator();
        if (pred.IsCrossColumn() or !pred.IsCrossTuple() or op != OperatorType::kEqual)
            return false;
    }

    return true;
}

bool DCVerifier::CheckOneInequality(const DC& dc) {
    std::vector<Predicate> predicates = dc.GetPredicates();
    size_t count_eq = 0, count_ineq = 0;

    for (Predicate const& pred : predicates) {
        Operator op = pred.GetOperator();

        if (op == OperatorType::kEqual and !pred.IsCrossColumn())
            count_eq++;
        else if (op != OperatorType::kEqual and op != OperatorType::kUnequal and
                 pred.IsCrossTuple())
            count_ineq++;
    }

    return count_eq + count_ineq == predicates.size() && count_ineq == 1;
}

bool DCVerifier::CheckOneTuple(const DC& dc) {
    for (auto const& pred : dc.GetPredicates()) {
        if (pred.IsCrossTuple()) return false;
    }

    return true;
}

bool DCVerifier::CheckTwoTuples(const DC& dc) {
    std::vector<Predicate> predicates = dc.GetPredicates();
    for (auto const& pred : predicates) {
        if (!pred.IsCrossTuple()) return false;
    }

    return true;
}

std::pair<util::Rect<point>, util::Rect<point>> DCVerifier::SearchRanges(
        const DC& dc, std::vector<std::byte const*> const& tuple) {
    std::vector<uint> ineq_cols = dc.GetColumnIndicesWithOperator([](Operator op) {
        return op.GetType() != OperatorType::kEqual and op.GetType() != OperatorType::kUnequal;
    });
    std::sort(ineq_cols.begin(), ineq_cols.end());

    // create n-dimensional vectors where each component has the
    // same type as the corresponding column which is in a
    // predicate with inequality operator
    std::vector<std::byte const*> empty_vec(tuple.size(), nullptr);
    point lower_bound = MakePoint(empty_vec, ineq_cols, ValType::kMinusInf);
    point upper_bound = MakePoint(empty_vec, ineq_cols, ValType::kPlusInf);
    point lower_bound_inv = lower_bound, upper_bound_inv = upper_bound;

    std::vector<Predicate> ineq_preds = dc.GetPredicates([](Predicate pred) {
        return pred.GetOperator().GetType() != OperatorType::kEqual and
               pred.GetOperator().GetType() != OperatorType::kUnequal and
               pred.GetLeftOperand().GetTuple() != pred.GetRightOperand().GetTuple();
    });

    using bound_type = util::Rect<point>::bound_type;

    std::vector<bound_type> lower_bound_type(ineq_cols.size(), bound_type::kClosed);
    std::vector<bound_type> upper_bound_type(ineq_cols.size(), bound_type::kClosed);
    std::vector<bound_type> lower_bound_type_inv(ineq_cols.size(), bound_type::kClosed);
    std::vector<bound_type> upper_bound_type_inv(ineq_cols.size(), bound_type::kClosed);

    for (size_t i = 0; i < ineq_preds.size(); i++) {
        OperatorType op_type = ineq_preds[i].GetOperator().GetType();
        uint left = ineq_preds[i].GetLeftOperand().GetColumn()->GetIndex();
        uint right = ineq_preds[i].GetRightOperand().GetColumn()->GetIndex();
        uint left_ind = 0, right_ind = 0;

        while (ineq_cols[left_ind] != left) left_ind++;
        while (ineq_cols[right_ind] != right) right_ind++;

        if (op_type == OperatorType::kLessEqual or op_type == OperatorType::kLess) {
            upper_bound[left_ind] = std::min(upper_bound[left_ind],
                                             Component(tuple[right], &data_[right].GetType()));
            lower_bound_inv[right_ind] = std::max(lower_bound_inv[right_ind],
                                                  Component(tuple[left], &data_[left].GetType()));
            if (op_type == OperatorType::kLess) {
                upper_bound_type[left_ind] = bound_type::kOpen;
                lower_bound_type_inv[right_ind] = bound_type::kOpen;
            }
        } else if (op_type == OperatorType::kGreaterEqual or op_type == OperatorType::kGreater) {
            lower_bound[left_ind] = std::max(lower_bound[left_ind],
                                             Component(tuple[right], &data_[right].GetType()));
            upper_bound_inv[right_ind] = std::min(upper_bound_inv[right_ind],
                                                  Component(tuple[left], &data_[left].GetType()));
            if (op_type == OperatorType::kGreater) {
                lower_bound_type[left_ind] = bound_type::kOpen;
                upper_bound_type_inv[right_ind] = bound_type::kOpen;
            }
        }
    }

    util::Rect<point> box(lower_bound, upper_bound), inv_box(lower_bound_inv, upper_bound_inv);
    box.SetBoundType(lower_bound_type, upper_bound_type);
    inv_box.SetBoundType(lower_bound_type_inv, upper_bound_type_inv);

    return {box, inv_box};
}

DC DCVerifier::ConvertEqualities(const DC& dc) {
    std::vector<Predicate> res, preds = dc.GetPredicates();
    for (auto const& pred : preds) {
        auto left = pred.GetLeftOperand();
        auto right = pred.GetRightOperand();
        if (pred.IsCrossColumn() and pred.IsCrossTuple() and
            pred.GetOperator().GetType() == OperatorType::kEqual) {
            auto left_op = ColumnOperand(left.GetColumn(), left.GetTuple());
            auto right_op = ColumnOperand(right.GetColumn(), right.GetTuple());
            auto less_equal = Operator(OperatorType::kLessEqual);
            auto greater_equal = Operator(OperatorType::kGreaterEqual);
            res.push_back({less_equal, left_op, right_op});
            res.push_back({greater_equal, left_op, right_op});
        } else {
            res.push_back(pred);
        }
    }

    return res;
}

std::vector<DC> DCVerifier::ConvertDisequalities(const DC& dc) {
    std::vector<DC> res;
    std::vector<Predicate> no_diseq_preds, diseq_preds, predicates = dc.GetPredicates();

    for (auto& pred : predicates) {
        auto op = pred.GetOperator();
        if (op.GetType() == OperatorType::kUnequal and pred.IsCrossTuple())
            diseq_preds.push_back(std::move(pred));
        else
            no_diseq_preds.push_back(std::move(pred));
    }

    std::vector<std::vector<Predicate>> temp_preds{no_diseq_preds};
    for (auto& pred : diseq_preds) {
        size_t size = temp_preds.size();
        for (size_t i = 0; i < size; i++) {
            temp_preds.push_back(temp_preds[i]);
        }

        ColumnOperand left = pred.GetLeftOperand();
        ColumnOperand right = pred.GetRightOperand();
        auto less_pred = Predicate(Operator(OperatorType::kLess), left, right);
        auto greater_pred = Predicate(Operator(OperatorType::kGreater), left, right);

        for (size_t i = 0; i < temp_preds.size() / 2; i++) {
            temp_preds[i].push_back(less_pred);
        }

        for (size_t i = temp_preds.size() / 2; i < temp_preds.size(); i++) {
            temp_preds[i].push_back(greater_pred);
        }
    }

    for (auto& vec_pred : temp_preds) {
        res.push_back({std::make_move_iterator(vec_pred.begin()),
                       std::make_move_iterator(vec_pred.end())});
    }

    return res;
}

// FIX: DC may contain braces, numbers or spaces, naive separation doesn't belong here.
std::vector<Predicate> DCVerifier::SplitDC(std::string const& dc_string) {
    size_t ind, start = 0;
    std::string token, sep = " and ";
    std::vector<Predicate> predicates;
    while (true) {
        ind = dc_string.find(sep, start);
        token = dc_string.substr(start, ind - start);
        std::vector<std::string> predicate_parts;
        boost::split(predicate_parts, token, boost::is_any_of(" "));
        for (size_t i = 0; i < 3; i += 2) {
            std::replace_if(predicate_parts[i].begin(), predicate_parts[i].end(),
                            boost::is_any_of("!()"), ' ');
            boost::trim(predicate_parts[i]);
        }
        auto left_op = ColumnOperand(predicate_parts[0], *relation_->GetSchema());
        auto right_op = ColumnOperand(predicate_parts[2], *relation_->GetSchema());
        auto oper = Operator::kStringToOperatorType.at(frozen::string(predicate_parts[1]));
        predicates.push_back({oper, left_op, right_op});

        if (ind == std::string::npos) break;
        start = ind + sep.size();
    }

    return predicates;
}

std::vector<std::byte const*> DCVerifier::GetTuple(size_t row) {
    std::vector<std::byte const*> res;
    res.reserve(data_.size());
    for (auto const& col : data_) {
        res.push_back(col.GetValue(row));
    }

    return res;
}

bool DCVerifier::Eval(std::vector<std::byte const*> tuple, std::vector<Predicate> preds) {
    for (auto const& pred : preds) {
        size_t left_ind = pred.GetLeftOperand().GetColumn()->GetIndex();
        size_t right_ind = pred.GetRightOperand().GetColumn()->GetIndex();
        Component left(tuple[left_ind], &data_[left_ind].GetType());
        Component right(tuple[right_ind], &data_[right_ind].GetType());
        if (!Component::Eval(left, right, pred.GetOperator())) return false;
    }

    return true;
}

point DCVerifier::MakePoint(std::vector<std::byte const*> const& vec,
                            std::vector<uint> const& indices, ValType val_type /* = kFinite */) {
    std::vector<Component> pt;
    for (auto ind : indices) {
        mo::Type const& type = data_[ind].GetType();
        pt.push_back(Component(vec[ind], &type, val_type));
    }

    return pt;
}

uint DCVerifier::HashTuple(std::vector<std::byte const*> const& vec,
                           std::vector<uint> const& indices) {
    std::vector<uint> res;
    res.reserve(vec.size());
    for (auto ind : indices) {
        mo::Type const& type = data_[ind].GetType();
        mo::Type::Hasher hasher = type.GetHasher();
        res.push_back(hasher(vec[ind]));
    }

    return boost::hash_value(res);
}

bool DCVerifier::ContainsNullOrEmpty(std::vector<uint> indices, size_t tuple_ind) const {
    for (auto ind : indices) {
        if (data_[ind].IsNullOrEmpty(tuple_ind)) return true;
    }
    return false;
}

DCType DCVerifier::GetType(const DC& dc) {
    if (CheckAllEquality(dc))
        return DCType::kAllEquality;
    else if (CheckOneInequality(dc))
        return DCType::kOneInequality;
    else if (CheckOneTuple(dc))
        return DCType::kOneTuple;
    else if (CheckTwoTuples(dc))
        return DCType::kTwoTuples;
    else
        return DCType::kMixed;
}

bool DCVerifier::DCHolds() {
    return result_;
}

}  // namespace algos