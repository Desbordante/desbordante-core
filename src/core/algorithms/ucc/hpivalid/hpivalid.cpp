#include "algorithms/ucc/hpivalid/hpivalid.h"

#include <deque>
#include <utility>
#include <vector>

#include "algorithms/fd/hycommon/preprocessor.h"
#include "algorithms/fd/hycommon/types.h"
#include "algorithms/ucc/hpivalid/config.h"
#include "algorithms/ucc/hpivalid/result_collector.h"
#include "algorithms/ucc/hpivalid/tree_search.h"
#include "util/logger.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos {

void HPIValid::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: UCC mining is meaningless.");
    }
}

hpiv::PLITable HPIValid::Preprocess(hpiv::ResultCollector& rc) {
    rc.StartTimer(hpiv::timer::TimerName::construct_clusters);

    hpiv::PLITable tab;

    tab.nr_rows = relation_->GetNumRows();
    tab.nr_cols = relation_->GetNumColumns();

    model::ColumnIndex const num_columns = relation_->GetNumColumns();
    auto plis = hy::util::BuildPLIs(relation_.get());
    for (model::ColumnIndex column_index = 0; column_index < num_columns; column_index++) {
        std::deque<model::PLI::Cluster> const& index = plis[column_index]->GetIndex();
        tab.plis.push_back(index);
    }
    tab.inverse_mapping = hy::util::BuildInvertedPlis(plis);

    rc.StopTimer(hpiv::timer::TimerName::construct_clusters);
    return tab;
}

unsigned long long HPIValid::ExecuteInternal() {
    hpiv::Config cfg;
    hpiv::ResultCollector rc(3600);

    rc.StartTimer(hpiv::timer::TimerName::total);
    hpiv::PLITable tab = Preprocess(rc);

    rc.StartTimer(hpiv::timer::TimerName::total_enum_algo);

    hpiv::TreeSearch tree_search(tab, cfg, rc);
    tree_search.Run();

    rc.StopTimer(hpiv::timer::TimerName::total_enum_algo);

    RegisterUCCs(rc);

    PrintInfo(rc);

    rc.StopTimer(hpiv::timer::TimerName::total);
    LOG_INFO("Elapsed time: {}", rc.Time(hpiv::timer::TimerName::total));

    return rc.Time(hpiv::timer::TimerName::total);
}

void HPIValid::RegisterUCCs(hpiv::ResultCollector const& rc) {
    std::vector<model::RawUCC> ucc_vector = rc.GetUCCs();
    std::shared_ptr<RelationalSchema const> const& schema = relation_->GetSharedPtrSchema();
    for (auto&& ucc : ucc_vector) {
        ucc_collection_.Register(schema, std::move(ucc));
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
