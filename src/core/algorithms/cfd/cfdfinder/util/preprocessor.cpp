#include "algorithms/cfd/cfdfinder/util/preprocessor.h"

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

std::tuple<hy::PLIs, hy::Columns, hy::Rows> Preprocess(CFDFinderRelationData* relation) {
    hy::PLIs plis = BuildPLIs(relation);

    auto inverted_plis = hy::util::BuildInvertedPlis(plis);

    auto compressed_records = hy::util::BuildRecordRepresentation(inverted_plis);

    return std::make_tuple(std::move(plis), std::move(inverted_plis),
                           std::move(compressed_records));
}

}  // namespace algos::cfdfinder