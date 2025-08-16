#include "algorithms/ucc/hpivalid/hpivalid.h"

#include <deque>      // for deque
#include <list>       // for _List_const_it...
#include <stdexcept>  // for runtime_error
#include <utility>    // for move
#include <vector>     // for vector

#include <boost/move/utility_core.hpp>  // for move
#include <easylogging++.h>              // for Writer, LOG

#include "algorithms/fd/hycommon/preprocessor.h"       // for BuildInvertedPlis
#include "algorithms/ucc/hpivalid/config.h"            // for Config
#include "algorithms/ucc/hpivalid/result_collector.h"  // for ResultCollector
#include "algorithms/ucc/hpivalid/tree_search.h"       // for TreeSearch
#include "primitive_collection.h"                      // for PrimitiveColle...
#include "table/column_index.h"                        // for ColumnIndex
#include "table/column_layout_relation_data.h"         // for ColumnLayoutRe...
#include "table/position_list_index.h"                 // for PLI, PositionL...
#include "ucc/hpivalid/enums.h"                        // for TimerName
#include "ucc/hpivalid/pli_table.h"                    // for PLITable
#include "ucc/raw_ucc.h"                               // for RawUCC
#include "ucc/ucc.h"                                   // for UCC

class RelationalSchema;

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
    LOG(INFO) << "Elapsed time: " << rc.Time(hpiv::timer::TimerName::total);

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
    LOG(INFO) << "Minimal UCCs: " << rc.UCCs();
    LOG(DEBUG) << "Mined UCCs:";
    for (model::UCC const& ucc : UCCList()) {
        LOG(DEBUG) << ucc.ToString();
    }
    LOG(INFO) << "Minimal difference sets: " << rc.DiffSetsFinal();
    LOG(INFO) << "Sampled difference sets: " << rc.DiffSets()
              << " (initial: " << rc.DiffSetsInitial() << ")";
    LOG(INFO) << "PLI intersections: " << rc.Intersections();
    LOG(INFO) << "Tree size: " << rc.TreeNodes();
}

}  // namespace algos
