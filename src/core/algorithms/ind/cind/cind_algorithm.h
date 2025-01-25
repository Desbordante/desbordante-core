#pragma once
#include <memory>

#include <ind/spider/spider.h>

#include "algorithms/algorithm.h"

namespace algos::cind {
class Cind final : public Algorithm {
public:
    explicit Cind(std::vector<std::string_view> phase_names = {});

    unsigned long long TimeTaken() const {
        return time_;
    }
    // using IND = model::IND;
    // std::list<IND> const& AINDList() const noexcept {
    //     return spider_algo_->INDList();
    // }



private:
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;
    void ResetState() final;

    void RegisterOptions();
    void MakeLoadOptsAvailable();
    // void MakeExecuteOptsAvailable() final;
    // bool SetExternalOption(std::string_view option_name, boost::any const& value) final;

private:
//     // algos
//     std::unique_ptr<Spider> spider_algo_;

    unsigned long long time_;
    
    // std::shared_ptr<std::vector<model::ColumnDomain>> domains_;

    // // Common load options
    // config::InputTables input_tables_;
    // config::EqNullsType is_null_equal_null_;
    // // Spider load options
    // config::ThreadNumType spider_threads_num_;
    // config::MemLimitMBType spider_mem_limit_mb_;
};

}  // namespace algos::cind