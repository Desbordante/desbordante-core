#include "core/algorithms/cfd/cfdfinder/model/hyfd/preprocessor.h"

#include <iterator>
#include <ranges>

#include "core/algorithms/fd/hycommon/preprocessor.h"

namespace {
using PLIs = algos::cfdfinder::PLIs;

PLIs BuildPLIs(algos::cfdfinder::CFDFinderRelationData* relation) {
    auto const& col_data = relation->GetColumnData();
    PLIs plis;
    plis.reserve(col_data.size());
    std::ranges::transform(col_data, std::back_inserter(plis), [](auto const& column_data) {
        return *column_data.GetPositionListIndex();
    });
    return plis;
}
}  // namespace

namespace algos::cfdfinder {

std::tuple<PLIsPtr, ColumnsPtr, RowsPtr> Preprocess(CFDFinderRelationData* relation) {
    auto plis = std::make_shared<PLIs>(BuildPLIs(relation));
    auto inverted_plis = std::make_shared<Columns>(hy::util::BuildInvertedPlis(*plis));
    auto compressed_records =
            std::make_shared<Rows>(hy::util::BuildRecordRepresentation(*inverted_plis));

    return std::make_tuple(std::move(plis), std::move(inverted_plis),
                           std::move(compressed_records));
}

}  // namespace algos::cfdfinder
