#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "config/tabular_data/input_table/option.h"
#include "dc.h"
#include "predicate_provider.h"
#include "table/typed_column_data.h"

namespace algos {

class DCVerification : public Algorithm {
private:
    DC dc_;
    bool result_;
    config::InputTable input_table_;
    std::vector<model::TypedColumnData> data_;

public:
    DCVerification();

    void ConvertToInequality();
    bool VerifyDC();

    // Verify DC in case if it contains only one heterogeneous or homogeneous inequality
    // (>, >=, <=, <) predicate and others predicates are only homogeneous equality ones
    bool VerifyOneInequality();
    bool CheckOneInequality();

    bool DCHolds() {
        return result_;
    };

    void ResetState() final;
    void LoadDataInternal() final;

    unsigned long long int ExecuteInternal() final;
};

}  // namespace algos