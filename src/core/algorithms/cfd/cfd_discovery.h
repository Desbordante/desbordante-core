#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <mutex>
#include <vector>

#include <boost/any.hpp>

#include "algorithms/algorithm.h"
#include "algorithms/cfd/model/cfd_relation_data.h"
#include "algorithms/cfd/model/cfd_types.h"
#include "config/tabular_data/input_table_type.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class CFDDiscovery : public Algorithm {
private:
    bool is_null_equal_null_;
    void RegisterOptions();
    void ResetState() final;
    virtual void ResetStateCFD() = 0;

protected:
    config::InputTable input_table_;

    unsigned columns_number_;
    unsigned tuples_number_;
    ItemsetCFDList cfd_list_;
    std::shared_ptr<CFDRelationData> relation_;

public:
    constexpr static std::string_view kDefaultPhaseName = "CFD mining";
    explicit CFDDiscovery(std::vector<std::string_view> phase_names);
    explicit CFDDiscovery();
    void LoadDataInternal() final;
    int NrCfds() const;
    ItemsetCFDList const& GetItemsetCfds() const;
    CFDList GetCfds() const;
    std::string GetRelationString(char delim = ' ') const;
    std::string GetRelationString(const SimpleTIdList& subset, char delim = ' ') const;
    std::string GetCfdString(ItemsetCFD const& cfd) const;
};
}  // namespace algos::cfd
