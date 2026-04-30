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
#include "core/util/logger.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos {

void HPIValid::LoadDataInternal() {
    auto schema = std::make_shared<RelationalSchema>(input_table_->GetRelationName());
    for (std::size_t i = 0; i != tab_.nr_cols; ++i) {
        auto column = Column(schema.get(), input_table_->GetColumnName(i), i);
        schema->AppendColumn(std::move(column));
    }
    schema_ = std::move(schema);

    auto column_value_id_maps = util::CreateValueIdMap(*input_table_);
    tab_.nr_rows = column_value_id_maps.front().size();
    auto plis = util::CreateStrippedPartitions(column_value_id_maps);
    tab_.nr_cols = plis.size();
    for (model::PositionListIndex const& pli : plis) {
        std::deque<model::PLI::Cluster> const& index = pli.GetIndex();
        tab_.plis.push_back(index);
    }
    tab_.inverse_mapping = hy::util::BuildInvertedPlis(plis);
}

unsigned long long HPIValid::ExecuteInternal() {
    hpiv::Config cfg;
    hpiv::ResultCollector rc(3600);

    rc.SetStartTime();

    hpiv::TreeSearch tree_search(tab_, cfg, rc);
    tree_search.Run();

    RegisterUCCs(rc);

    PrintInfo(rc);

    return rc.GetTimeSinceStart();
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
