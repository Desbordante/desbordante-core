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
#include <easylogging++.h>

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "model/table/column_layout_relation_data.h"

namespace algos {

namespace mo = model;

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

    dc_ = ParseDCString(dc_string_);
    if (CheckOneOrZeroInequality())
        result_ = VerifyOneInequality();
    else {
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
    return true;
}

bool DCVerification::CheckOneOrZeroInequality() {
    std::vector<mo::Predicate> predicates = dc_.GetPredicates();
    size_t count_eq = 0, count_ineq = 0;

    for (mo::Predicate const& pred : predicates) {
        mo::Operator op = pred.GetOperator();
        mo::ColumnOperand l = pred.GetLeftOperand(), r = pred.GetRightOperand();

        if (op == mo::OperatorType::kEqual and l.GetColumn() == r.GetColumn())
            count_eq++;
        else if (op != mo::OperatorType::kEqual and op != mo::OperatorType::kUnequal)
            count_ineq++;
    }

    return count_eq + count_ineq == predicates.size() && count_ineq <= 1;
}

bool DCVerification::VerifyOneInequality() {
    const std::vector<unsigned> indices =
            dc_.GetColumnIndicesWithOperator(mo::OperatorType::kEqual);
    std::vector<mo::Predicate> predicates = dc_.GetPredicates();
    mo::PredicatePtr ineq_pred = nullptr;
    for (const mo::Predicate& pred : predicates) {
        auto pred_type = pred.GetOperator().GetType();
        if (pred_type != mo::OperatorType::kEqual and pred_type != mo::OperatorType::kUnequal)
            ineq_pred = &pred;
    }

    for (auto ind : indices) {
        mo::TypeId typeId = data_[ind].GetTypeId();
        if (!mo::Type::IsOrdered(typeId)) return false;
    }

    mo::ColumnOperand operandA = ineq_pred->GetLeftOperand();
    mo::ColumnOperand operandB = ineq_pred->GetRightOperand();
    unsigned indA = operandA.GetColumn()->GetIndex();
    unsigned indB = operandB.GetColumn()->GetIndex();
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

    std::unordered_map<unsigned, std::byte const*> minA, minB, maxA, maxB;

    for (size_t i = 0; i < data_.size(); i++) {
        std::vector<std::byte const*> tuple = GetTuple(indices, i);
        std::vector<unsigned> hash_tuple = ByteVecToUnsignedVec(tuple, indices);
        unsigned key = boost::hash_value(hash_tuple);

        // TODO: convert each value into res_type value
        if (minA.find(key) == minA.end()) {
            minA[key] = maxA[key] = nullptr;
            minB[key] = maxB[key] = nullptr;
        } else if (minA[key] != nullptr) {
            if (ineq_pred == nullptr)
                if ((op.GetType() == mo::OperatorType::kLess or
                     op.GetType() == mo::OperatorType::kLessEqual) and
                    (op.Eval(minA[key], tuple[indB], *res_type) or
                     op.Eval(tuple[indA], maxB[key], *res_type)))
                    return false;

            if ((op.GetType() == mo::OperatorType::kGreater or
                 op.GetType() == mo::OperatorType::kGreaterEqual) and
                (op.Eval(maxA[key], tuple[indB], *res_type) or
                 op.Eval(tuple[indA], minB[key], *res_type)))
                return false;
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
    return true;
}

std::vector<unsigned> DCVerification::ByteVecToUnsignedVec(const std::vector<std::byte const*> vec,
                                                           std::vector<unsigned> const& indices) {
    std::vector<unsigned> res(vec.size());
    for (auto ind : indices) {
        mo::Type const& type = data_[ind].GetType();
        mo::Type::Hasher hasher = type.GetHasher();
        res.push_back(hasher(vec[ind]));
    }

    return res;
}

mo::DC DCVerification::ParseDCString(std::string dc_string) {
    using namespace boost::algorithm;
    std::vector<mo::Predicate> predicates;
    std::vector<std::string> tokens;
    boost::split(tokens, dc_string, boost::is_any_of("and"),
                 token_compress_mode_type::token_compress_on);

    for (auto& token : tokens) {
        std::replace_if(token.begin(), token.end(), boost::is_any_of("!()"), ' ');
        boost::trim(token);
        LOG(INFO) << token;
        std::vector<std::string> predicate_parts;
        boost::split(predicate_parts, token, boost::is_any_of(" "));
        predicates.push_back(
                mo::Predicate{mo::Operator::kStringToOperatorMap.at(predicate_parts[1]),
                              mo::ColumnOperand(predicate_parts[0], *relation_->GetSchema()),
                              mo::ColumnOperand(predicate_parts[2], *relation_->GetSchema())});
    }

    return {predicates};
}

std::vector<std::byte const*> DCVerification::GetTuple(std::vector<unsigned> const& indices,
                                                       size_t row) {
    std::vector<std::byte const*> res;
    for (auto ind : indices) {
        res.push_back(data_[ind].GetValue(row));
    }
    return res;
}

}  // namespace algos