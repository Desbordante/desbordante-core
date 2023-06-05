#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <mutex>
#include <vector>

#include <boost/any.hpp>

#include "algorithms/algorithm.h"
#include "algorithms/cfd/structures/cfd_relation_data.h"
#include "algorithms/cfd/structures/cfd_types.h"
#include "util/config/tabular_data/input_table_type.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class CFDDiscovery : public Algorithm {
private:
    bool is_null_equal_null_;
    void RegisterOptions();
    void ResetState() final;
    virtual void ResetStateCFD() = 0;

protected:
    util::config::InputTable input_table_;

    unsigned columns_number_;
    unsigned tuples_number_;
    CFDList cfd_list_;
    std::shared_ptr<CFDRelationData> relation_;

public:
    constexpr static std::string_view kDefaultPhaseName = "CFD mining";
    explicit CFDDiscovery(std::vector<std::string_view> phase_names);
    explicit CFDDiscovery();
    void LoadDataInternal() final;
    int NrCfds() const;
    CFDList GetCfds() const;
    std::string GetRelationString(char delim = ' ') const;
    std::string GetRelationString(const SimpleTIdList& subset, char delim = ' ') const;
    std::string GetCfdString(CFD const& cfd) const;
};
}  // namespace algos::cfd
