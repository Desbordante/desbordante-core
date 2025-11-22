#include "algorithms/cfd/cfdfinder/model/hyfd/preprocessor.h"

#include "algorithms/fd/hycommon/preprocessor.h"

namespace {
using PLIs = algos::cfdfinder::PLIs;

PLIs BuildPLIs(algos::cfdfinder::CFDFinderRelationData* relation) {
    auto& col_data = relation->GetColumnData();
    PLIs plis(col_data.size());
    std::ranges::transform(col_data, plis.begin(),
                           [](auto& column_data) { return column_data.GetPositionListIndex(); });
    return plis;
}
}  // namespace

namespace algos::cfdfinder {

std::tuple<PLIs, Columns, Rows> Preprocess(CFDFinderRelationData* relation) {
    PLIs plis = BuildPLIs(relation);
    Columns inverted_plis = hy::util::BuildInvertedPlis(plis);
    Rows compressed_records = hy::util::BuildRecordRepresentation(inverted_plis);

    return std::make_tuple(std::move(plis), std::move(inverted_plis),
                           std::move(compressed_records));
}

}  // namespace algos::cfdfinder
