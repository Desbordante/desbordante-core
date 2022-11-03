#pragma once

#include <queue>
#include <vector>

#include "hyfd_config.h"
#include "structures/non_fds.h"
#include "types.h"
#include "util/position_list_index.h"

namespace algos::hyfd {

class Sampler {
private:
    class Efficiency;
    double efficiency_threshold_ = HyFDConfig::kEfficiencyThreshold;

    PLIs plis_;
    RowsPtr compressed_records_;
    std::priority_queue<Efficiency> efficiency_queue_;

    std::shared_ptr<NonFds> non_fds_;

    void ProcessComparisonSuggestions(IdPairs const& comparison_suggestions);
    void InitializeEfficiencyQueue();

    void Match(boost::dynamic_bitset<>& attributes, size_t first_record_id,
               size_t second_record_id);
    void RunWindow(Efficiency& efficiency, util::PositionListIndex const& pli);

public:
    Sampler(PLIs const& plis, RowsPtr pli_records);
    ~Sampler();

    NonFDList GetNonFDCandidates(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hyfd
