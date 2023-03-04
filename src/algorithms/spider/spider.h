#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include <boost/mp11.hpp>
#include <boost/mp11/algorithm.hpp>

#include "algorithms/primitive.h"
#include "attribute.h"
#include "id_algorithm.h"
#include "model/cursor.h"
#include "table_processor.h"

namespace algos {

class Spider : public IDAlgorithm {
public:
    using UID = std::pair<std::size_t, std::size_t>;
    using AttrPtr = Attribute*;
    using AttrMap = Attribute::AttrMap;

private:
    std::unique_ptr<BaseTableProcessor> CreateChunkProcessor(std::filesystem::path const& path,
                                                             SpilledFilesManager& manager) const;

    std::filesystem::path temp_dir_ = "temp";
    std::size_t ram_limit_ = 2 << 30;
    std::size_t mem_check_frequency_ = 100000;
    ushort threads_count_ = 1;
    ColType col_type_ = +ColType::SET;
    KeyType key_type_ = +KeyType::STRING_VIEW;

    static config::OptionType<decltype(temp_dir_)> TempOpt;
    static config::OptionType<decltype(ram_limit_)> MemoryLimitOpt;
    static config::OptionType<decltype(mem_check_frequency_)> MemoryCheckFreq;
    static config::OptionType<decltype(col_type_)> ColTypeOpt;
    static config::OptionType<decltype(key_type_)> ValueTypeOpt;

    std::vector<UID> result_;
    AttrMap attrs_;
    std::priority_queue<AttrPtr, std::vector<AttrPtr>, std::function<bool(AttrPtr, AttrPtr)>>
            attribute_queue_{[](auto lhs, auto rhs) { return lhs->CompareTo(*rhs) >= 0; }};
    struct InnerState {
        std::size_t n_cols = 0;
        std::vector<std::string> max_values{};
        std::vector<std::size_t> number_of_columns{};
        std::vector<std::size_t> table_column_start_indexes{};
    } state_;

    void RegisterOptions();

    void ResetState() final {
        result_.clear();
        attrs_.clear();
    }

protected:
    unsigned long long ExecuteInternal() final;
    void PreprocessData();
    void InitializeAttributes();
    void ComputeUIDs();
    void Output();
    void RegisterUID(UID uid);

    void PrintResult(std::ostream& out) const;
    void MakeExecuteOptsAvailable() final;

public:
    explicit Spider() : IDAlgorithm({}) {
        RegisterOptions();
    }

    const std::vector<UID>& GetUIDs() const {
        return result_;
    }
};

}  // namespace algos
