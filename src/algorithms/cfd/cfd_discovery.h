#pragma once

// see ./LICENSE

#include <boost/any.hpp>
#include <filesystem>
#include <list>
#include <map>
#include <mutex>
#include <vector>

#include "algorithms/options/type.h"
#include "model/cfd.h"
#include "algorithms/cfd/util/prefix_tree.h"
#include "algorithms/primitive.h"
#include "model/cfd_relation_data.h"
#include "algorithms/cfd/generator_store.h"
#include "algorithms/cfd/miner_node.h"
#include "algorithms/cfd/partition_table.h"
#include "algorithms/cfd/enums.h"


namespace algos {

class CFDDiscovery : public algos::Primitive {
private:
    bool is_null_equal_null_;
    void RegisterOptions();
    void ResetState() final;
    virtual void ResetStateCFD() = 0;

protected:
    unsigned columns_number_;
    unsigned tuples_number_;
    CFDList cfd_list_;
    std::shared_ptr<CFDRelationData> relation_;

    static const config::OptionType<decltype(columns_number_)> ColumnsNumberOpt;
    static const config::OptionType<decltype(tuples_number_)> TuplesNumberOpt;

public:
    constexpr static std::string_view kDefaultPhaseName = "CFD mining";
    explicit CFDDiscovery(std::vector<std::string_view> phase_names);
    explicit CFDDiscovery();
    void FitInternal(model::IDatasetStream& data_stream) final;
    int NrCfds() const;
    CFDList GetCfds() const;
    std::string GetRelationString(char delim = ' ') const;
    std::string GetRelationString(const SimpleTidList& subset, char delim= ' ') const;
    std::string GetCfdString(CFD const& cfd) const;
};
}
