#include "test_ind_util.h"

#include <algorithm>
#include <sstream>

namespace tests {

std::string TableNamesToString(CSVConfigs const& csv_configs) {
    std::stringstream ss;
    for (auto const& csv_config : csv_configs) {
        ss << csv_config.path.filename() << " ";
    }
    return ss.str();
}

INDTest ToINDTest(model::IND const& ind) {
    auto to_cc_test = [](model::ColumnCombination const& cc) {
        return std::make_pair(cc.GetTableIndex(), cc.GetColumnIndices());
    };
    return std::make_pair(to_cc_test(ind.GetLhs()), to_cc_test(ind.GetRhs()));
}

std::vector<INDTest> ToSortedINDTestVec(std::list<model::IND> const& inds) {
    std::vector<INDTest> ind_test_vec;
    ind_test_vec.reserve(inds.size());
    std::transform(inds.begin(), inds.end(), std::back_inserter(ind_test_vec), ToINDTest);
    std::sort(ind_test_vec.begin(), ind_test_vec.end());
    return ind_test_vec;
}

}  // namespace tests
