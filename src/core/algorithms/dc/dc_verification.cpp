#include "dc_verification.h"

#include <algorithm>
#include <ctime>
#include <iostream>

#include <easylogging++.h>

namespace algos {

namespace mo = model;

DCVerification::DCVerification() : Algorithm({}) {
    RegisterOption(config::kTableOpt(&input_table_));
    MakeOptionsAvailable({config::kTableOpt.GetName()});
};

void DCVerification::LoadDataInternal() {
    data_ = model::CreateTypedColumnData(*input_table_, true);
    std::cout << data_[0].GetNumRows();
}

void DCVerification::ResetState() {}

bool DCVerification::VerifyDC() {
    return true;
}

bool DCVerification::VerifyOneInequality() {
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
        else if (op != mo::OperatorType::kEqual and op != mo::OperatorType::kUnequal)
            count_ineq++;
    }

    return count_eq + count_ineq == predicates.size() && count_ineq <= 1;
}

void DCVerification::ConvertToInequality() {}

unsigned long long int DCVerification::ExecuteInternal() {
    auto start = std::chrono::system_clock::now();

    // ConvertToInequality();

    if (CheckOneInequality()) VerifyOneInequality();

    result_ = VerifyDC();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start);
    LOG(DEBUG) << "";

    return elapsed_time.count();
}

}  // namespace algos