#include "core/algorithms/ucc/hpivalid/hpivalid.h"

#include <deque>
#include <utility>
#include <vector>

#include "core/algorithms/fd/hycommon/preprocessor.h"
#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/ucc/hpivalid/config.h"
#include "core/algorithms/ucc/hpivalid/result_collector.h"
#include "core/algorithms/ucc/hpivalid/tree_search.h"
#include "core/model/table/create_stripped_partitions.h"
#include "core/model/table/to_value_id_mapped_columns.h"
#include "core/util/logger.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos {

void HPIValid::LoadDataInternal() {
    schema_ = RelationalSchema::CreateFrom(*input_table_);

    std::vector<std::vector<int>> value_id_mapped_columns =
            model::ToValueIdMappedColumns(*input_table_);
    if (value_id_mapped_columns.empty() || value_id_mapped_columns.front().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
    tab_.nr_cols = value_id_mapped_columns.size();
    tab_.nr_rows = value_id_mapped_columns.front().size();

    auto plis = model::CreateStrippedPartitions(value_id_mapped_columns);
    for (model::PositionListIndex const& pli : plis) {
        std::deque<model::PLI::Cluster> const& index = pli.GetIndex();
        tab_.plis.push_back(index);
    }
    tab_.inverse_mapping = hy::util::BuildInvertedPlis(plis);
}

void HPIValid::ExecuteInternal() {
    hpiv::Config cfg;
    hpiv::ResultCollector rc(3600);

    rc.SetStartTime();

    hpiv::TreeSearch tree_search(tab_, cfg, rc);
    tree_search.Run();

    RegisterUCCs(rc);

    PrintInfo(rc);
}

void HPIValid::RegisterUCCs(hpiv::ResultCollector const& rc) {
    std::vector<model::RawUCC> ucc_vector = rc.GetUCCs();
    for (auto&& ucc : ucc_vector) {
        ucc_collection_.Register(schema_, std::move(ucc));
    }
}

void HPIValid::PrintInfo(hpiv::ResultCollector const& rc) const {
    LOG_INFO("Minimal UCCs: {}", rc.UCCs());
    LOG_DEBUG("Mined UCCs:");
    for (model::UCC const& ucc : UCCList()) {
        LOG_DEBUG("{}", ucc.ToString());
    }
    LOG_INFO("Minimal difference sets: {}", rc.DiffSetsFinal());
    LOG_INFO("Sampled difference sets: {} (initial: {})", rc.DiffSets(), rc.DiffSetsInitial());
    LOG_INFO("PLI intersections: {}", rc.Intersections());
    LOG_INFO("Tree size: {}", rc.TreeNodes());
}

}  // namespace algos
