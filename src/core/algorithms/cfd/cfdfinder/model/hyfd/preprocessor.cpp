#include "algorithms/cfd/cfdfinder/model/hyfd/preprocessor.h"

#include "algorithms/fd/hycommon/preprocessor.h"

namespace {
using PLIs = algos::hy::PLIs;

PLIs BuildPLIs(algos::cfdfinder::CFDFinderRelationData* relation) {
    PLIs plis;
    std::transform(relation->GetColumnData().begin(), relation->GetColumnData().end(),
                   std::back_inserter(plis),
                   [](auto& column_data) { return column_data.GetPositionListIndex(); });
    return plis;
}
}  // namespace

namespace algos::cfdfinder {

std::tuple<hy::PLIsPtr, ColumnsPtr, hy::RowsPtr> Preprocess(CFDFinderRelationData* relation) {
    hy::PLIs plis = BuildPLIs(relation);
    auto inverted_plis = hy::util::BuildInvertedPlis(plis);
    auto compressed_records = hy::util::BuildRecordRepresentation(inverted_plis);

    return std::make_tuple(std::make_shared<hy::PLIs>(std::move(plis)),
                           std::make_shared<hy::Columns>(std::move(inverted_plis)),
                           std::make_shared<hy::Rows>(std::move(compressed_records)));
}

}  // namespace algos::cfdfinder