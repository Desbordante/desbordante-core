#include "dc_verification.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include "component.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "model/table/column_layout_relation_data.h"
#include "point.h"

namespace algos {

namespace mo = model;

using point = Point<Component>;

// using rtree = boost::geometry::index::rtree<point, boost::geometry::index::quadratic<10>>;

point DCVerification::MakePoint(std::vector<std::byte const*> const& vec,
                                std::vector<uint> const& indices,
                                ValType val_type /* = kFinite */) {
    std::vector<Component> pt;
    for (auto ind : indices) {
        mo::Type const& type = data_[ind].GetType();
        pt.push_back(Component(vec[ind], &type, val_type));
    }

    return pt;
}

uint DCVerification::HashTuple(std::vector<std::byte const*> const& vec,
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

DCVerification::DCVerification() : Algorithm({}) {
    using namespace config::names;

    RegisterOptions();
    MakeOptionsAvailable({kTable});
};

void DCVerification::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option<std::string>(&dc_string_, kDenialConstraint, kDDenialConstraint, ""));
    RegisterOption(config::kTableOpt(&input_table_));
}

void DCVerification::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kDenialConstraint});
}

void DCVerification::LoadDataInternal() {
    data_ = model::CreateTypedColumnData(*input_table_, true);
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, true);
}

void DCVerification::ResetState() {}

unsigned long long int DCVerification::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();
    // ConvertToInequality();

    dc_ = SplitDC(dc_string_);
    if (CheckAllEquality()) {
        result_ = VerifyAllEquality();
    } else if (CheckOneInequality()) {
        result_ = VerifyOneInequality();
    } else {
        result_ = VerifyDC();
    }

    ConvertToInequality();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);
    LOG(DEBUG) << "";

    return elapsed_time.count();
}

void DCVerification::ConvertToInequality() {}

bool DCVerification::VerifyDC() {
    using namespace kdt;

    std::vector<uint> eq_cols = dc_.GetColumnIndicesWithOperator(
            [](mo::Operator op) { return op.GetType() == mo::OperatorType::kEqual; });

    std::unordered_map<uint, kdtree<point>> hash;
    std::vector<point> points;
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);
        std::vector<uint> ineq_cols = dc_.GetColumnIndicesWithOperator([](mo::Operator op) {
            return op.GetType() != mo::OperatorType::kEqual and
                   op.GetType() != mo::OperatorType::kUnequal;
        });

        if (hash.find(key) == hash.end()) hash[key] = kdtree<point>();

        rect<point> box = SearchRange(tuple);
        rect<point> inv_box = InvertRange(box);

        if (hash[key].query_search(box).size() != 0 or hash[key].query_search(inv_box).size() != 0)
            return false;

        hash[key].insert(MakePoint(tuple, ineq_cols));
    }

    return true;
}

kdt::rect<point> DCVerification::SearchRange(std::vector<std::byte const*> const& tuple) {
    std::vector<uint> ineq_cols = dc_.GetColumnIndicesWithOperator([](mo::Operator op) {
        return op.GetType() != mo::OperatorType::kEqual and
               op.GetType() != mo::OperatorType::kUnequal;
    });
    std::sort(ineq_cols.begin(), ineq_cols.end());

    // create n-dimensional vectors where each component has the
    // same type as the corresponding column which is in a
    // predicate with inequality operator
    std::vector<std::byte const*> empty_vec(tuple.size(), nullptr);
    point L = MakePoint(empty_vec, ineq_cols, kMinusInf);
    point U = MakePoint(empty_vec, ineq_cols, kPlusInf);

    std::vector<mo::Predicate> ineq_preds = dc_.GetPredicatesWithOperator([](mo::Operator op) {
        return op.GetType() != mo::OperatorType::kEqual and
               op.GetType() != mo::OperatorType::kUnequal;
    });

    using bound_type = kdt::rect<point>::bound_type;
    using enum kdt::rect<point>::bound_type;
    
    std::vector<bound_type> lower_bound_type(ineq_preds.size(), kClosed);
    std::vector<bound_type> upper_bound_type(ineq_preds.size(), kClosed);

    for (size_t i = 0; i < ineq_preds.size(); i++) {
        mo::OperatorType op_type = ineq_preds[i].GetOperator().GetType();
        uint left = ineq_preds[i].GetLeftOperand().GetColumn()->GetIndex();
        uint right = ineq_preds[i].GetRightOperand().GetColumn()->GetIndex();
        assert(left == right);  // DC should be row homogeneous

        if (op_type == mo::OperatorType::kLessEqual or op_type == mo::OperatorType::kLess) {
            U[i] = Component(tuple[left], &data_[left].GetType());
            if (op_type == mo::OperatorType::kLess) upper_bound_type[i] = kOpen;
        } else if (op_type == mo::OperatorType::kGreaterEqual or
                   op_type == mo::OperatorType::kGreater) {
            L[i] = Component(tuple[left], &data_[left].GetType());
            if (op_type == mo::OperatorType::kGreater) lower_bound_type[i] = kOpen;
        }
    }

    kdt::rect<point> box(L, U);
    box.set_bound_type(lower_bound_type, upper_bound_type);

    return box;
}

kdt::rect<point> DCVerification::InvertRange(kdt::rect<point> const& box) {
    point L = box.lower_bound_, U = box.upper_bound_;
    for (size_t i = 0; i < L.get_dim(); i++) {
        ValType& l_val_type = L[i].GetValType();
        ValType& u_val_type = U[i].GetValType();
        L[i].Swap(U[i]);

        if (l_val_type == kPlusInf) l_val_type = kMinusInf;
        if (u_val_type == kMinusInf) u_val_type = kPlusInf;
    }

    kdt::rect<point> res(L, U);
    res.set_bound_type(box.upper_bound_type_, box.lower_bound_type_);

    return res;
}

bool DCVerification::CheckAllEquality() {
    std::vector<mo::Predicate> predicates = dc_.GetPredicates();

    for (mo::Predicate const& pred : predicates) {
        mo::Operator op = pred.GetOperator();
        mo::ColumnOperand l = pred.GetLeftOperand(), r = pred.GetRightOperand();
        if (op != mo::OperatorType::kEqual or l.GetColumn() != r.GetColumn()) return false;
    }

    return true;
}

bool DCVerification::CheckOneInequality() {
    std::vector<mo::Predicate> predicates = dc_.GetPredicates();
    size_t count_eq = 0, count_ineq = 0;

    for (mo::Predicate const& pred : predicates) {
        mo::Operator op = pred.GetOperator();
        mo::ColumnOperand l = pred.GetLeftOperand(), r = pred.GetRightOperand();

        if (op == mo::OperatorType::kEqual and l.GetColumn() == r.GetColumn())
            count_eq++;
        else if (op != mo::OperatorType::kEqual and op != mo::OperatorType::kUnequal and
                 l.GetColumn() != r.GetColumn())
            count_ineq++;
    }

    return count_eq + count_ineq == predicates.size() && count_ineq == 1;
}

bool DCVerification::VerifyAllEquality() {
    std::vector<uint> const eq_cols = dc_.GetColumnIndicesWithOperator(
            [](mo::Operator op) { return op.GetType() == mo::OperatorType::kEqual; });

    std::unordered_set<uint> res;
    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);
        if (res.find(key) != res.end()) return false;
        res.insert(key);
    }

    return true;
}

bool DCVerification::VerifyOneInequality() {
    std::vector<mo::Predicate> predicates = dc_.GetPredicates();
    mo::PredicatePtr ineq_pred = nullptr;
    for (mo::Predicate const& pred : predicates) {
        auto pred_type = pred.GetOperator().GetType();
        if (pred_type != mo::OperatorType::kEqual and pred_type != mo::OperatorType::kUnequal)
            ineq_pred = &pred;
    }

    mo::ColumnOperand operandA = ineq_pred->GetLeftOperand();
    mo::ColumnOperand operandB = ineq_pred->GetRightOperand();
    uint indA = operandA.GetColumn()->GetIndex();
    uint indB = operandB.GetColumn()->GetIndex();
    mo::TypedColumnData& colA = data_[indA];
    mo::TypedColumnData& colB = data_[indB];
    mo::Operator op = ineq_pred->GetOperator();
    mo::Type const& typeA = data_[indA].GetType();
    mo::Type const& typeB = data_[indB].GetType();
    mo::Type const* res_type = &typeA;

    if (colA.IsNumeric() and colB.IsNumeric()) {
        if (typeA.GetTypeId() == +mo::TypeId::kDouble or typeB.GetTypeId() == +mo::TypeId::kDouble)
            res_type = new mo::DoubleType();
        else
            res_type = new mo::IntType();
    } else if (typeA.GetTypeId() == +mo::TypeId::kString and
               typeB.GetTypeId() == +mo::TypeId::kString) {
        res_type = new mo::StringType();
    } else
        return false;

    std::vector<uint> const eq_cols = dc_.GetColumnIndicesWithOperator(
            [](mo::Operator op) { return op.GetType() == mo::OperatorType::kEqual; });
    std::unordered_map<uint, std::byte const*> minA, minB, maxA, maxB;

    for (size_t i = 0; i < data_[0].GetNumRows(); i++) {
        std::vector<std::byte const*> tuple = GetTuple(i);
        uint key = HashTuple(tuple, eq_cols);

        // TODO: convert each value into res_type value
        if (minA.find(key) == minA.end()) {
            minA[key] = maxA[key] = nullptr;
            minB[key] = maxB[key] = nullptr;
        }

        if (minA[key] != nullptr) {
            if ((op.GetType() == mo::OperatorType::kLess or
                 op.GetType() == mo::OperatorType::kLessEqual) and
                (op.Eval(minA[key], tuple[indB], *res_type) or
                 op.Eval(tuple[indA], maxB[key], *res_type))) {
                LOG(INFO) << "DC Verification fails at row number " << i + 2;
                return false;
            }

            else if ((op.GetType() == mo::OperatorType::kGreater or
                      op.GetType() == mo::OperatorType::kGreaterEqual) and
                     (op.Eval(maxA[key], tuple[indB], *res_type) or
                      op.Eval(tuple[indA], minB[key], *res_type))) {
                LOG(INFO) << "DC Verification fails at row number " << i + 2;
                return false;
            }

            minA[key] = std::min(minA[key], tuple[indA], typeA.GetComparator());
            minB[key] = std::min(minB[key], tuple[indB], typeB.GetComparator());
            maxA[key] = std::max(maxA[key], tuple[indA], typeA.GetComparator());
            maxB[key] = std::max(maxB[key], tuple[indB], typeB.GetComparator());
        } else {
            minA[key] = tuple[indA];
            minB[key] = tuple[indB];
            maxA[key] = tuple[indA];
            maxB[key] = tuple[indB];
        }
    }
    delete res_type;

    return true;
}

std::vector<mo::Predicate> DCVerification::SplitDC(std::string dc_string) {
    size_t ind, start = 0;
    std::string token, sep = " and ";
    std::vector<mo::Predicate> predicates;
    while (true) {
        ind = dc_string.find(sep, start);
        token = dc_string.substr(start, ind - start);
        std::replace_if(token.begin(), token.end(), boost::is_any_of("!()"), ' ');
        boost::trim(token);
        std::vector<std::string> predicate_parts;
        boost::split(predicate_parts, token, boost::is_any_of(" "));
        predicates.push_back(
                mo::Predicate{mo::Operator::kStringToOperatorMap.at(predicate_parts[1]),
                              mo::ColumnOperand(predicate_parts[0], *relation_->GetSchema()),
                              mo::ColumnOperand(predicate_parts[2], *relation_->GetSchema())});
        if (ind == std::string::npos) break;
        start = ind + sep.size();
    }

    return predicates;
}

std::vector<std::byte const*> DCVerification::GetTuple(size_t row) {
    std::vector<std::byte const*> res;
    res.reserve(data_.size());
    for (auto const& col : data_) {
        res.push_back(col.GetValue(row));
    }

    return res;
}

}  // namespace algos