#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <mutex>
#include <vector>

#include <boost/any.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/cfd/model/cfd_relation_data.h"
#include "core/algorithms/cfd/model/cfd_types.h"
#include "core/config/tabular_data/input_table_type.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class CFDDiscovery : public Algorithm {
private:
    void RegisterOptions();
    void ResetState() final;
    virtual void ResetStateCFD() = 0;

protected:
    config::InputTable input_table_;

    CFDList cfd_list_;
    std::shared_ptr<CFDRelationData> relation_;

public:
    explicit CFDDiscovery();
    void LoadDataInternal() final;
    CFDList const& GetCfds() const;
    std::string GetCfdString(ItemsetCFD const& cfd) const;
};
}  // namespace algos::cfd
