#pragma once

#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>

#include <easylogging++.h>

#include "algorithms/ind_algorithm.h"
#include "algorithms/options/thread_number_opt.h"
#include "attribute.h"
#include "enums.h"
#include "preprocessing/sorted_column_writer.h"
#include "preprocessing/table_processor.h"
#include "type.h"

namespace algos {

class Spider : public INDAlgorithm<Primitive> {
    using UIND = std::pair<std::size_t, std::size_t>;
    using Attribute = ind::Attribute;
    using AttrPtr = Attribute*;
    using AttrMap = Attribute::AttrMap;
    using ColType = ind::ColType;
    using KeyType = ind::KeyType;

private:
    std::unique_ptr<ind::preproc::BaseTableProcessor> CreateChunkProcessor(
            model::IDatasetStream::DataInfo const& data_info,
            ind::preproc::SortedColumnWriter& writer) const;

    std::filesystem::path temp_dir_ = "temp";
    std::size_t mem_limit_mb_;
    std::size_t mem_check_frequency_;
    config::ThreadNumType threads_count_ = 1;
    ColType col_type_ = +ColType::VECTOR;
    KeyType key_type_ = +KeyType::STRING_VIEW;

    static config::OptionType<decltype(temp_dir_)> TempOpt;
    static config::OptionType<decltype(mem_limit_mb_)> MemoryLimitMBOpt;
    static config::OptionType<decltype(mem_check_frequency_)> MemoryCheckFreq;
    static config::OptionType<decltype(col_type_)> ColTypeOpt;
    static config::OptionType<decltype(key_type_)> KeyTypeOpt;

    /* Preprocessed table information struct:
     * contains metadata about preprocessed tables
     */
    struct TablesStats {
        std::size_t n_cols = 0;
        std::vector<std::string> max_values{};
        std::vector<std::size_t> number_of_columns{};
        std::vector<std::filesystem::path> attribute_paths{};
        DatasetsOrder datasets_order{};
    } stats_;

    /* Struct that stores information about the execution time of each stage of the algorithm.
     */
    struct StageTimer {
        std::chrono::milliseconds preprocessing;
        std::chrono::milliseconds initializing;
        std::chrono::milliseconds computing;

        void Print() const {
            LOG(INFO) << "PreprocessingTime: " << preprocessing.count();
            LOG(INFO) << "InitializeTime: " << initializing.count();
            LOG(INFO) << "ComputingTime: " << computing.count();
        }
    } timings_;

    std::list<UIND> uinds_;
    AttrMap attrs_;
    std::priority_queue<AttrPtr, std::vector<AttrPtr>, std::function<bool(AttrPtr, AttrPtr)>>
            attribute_queue_{[](auto lhs, auto rhs) { return lhs->CompareTo(*rhs) >= 0; }};

    void RegisterOptions();
    void MakePreprocessOptsAvailable();

    void ResetState() final {
        uinds_.clear();
        attrs_.clear();
    }
    std::size_t GetMemoryLimitInBytes() const {
        return mem_limit_mb_ << 20;
    }
    IND::ColumnCombination GetCCByID(unsigned id) const {
        unsigned table = 0;
        while (stats_.number_of_columns[table] <= id) {
            id -= stats_.number_of_columns[table++];
        }
        return {.table_index = table, .column_indices = {id}};
    }

protected:
    unsigned long long ExecuteInternal() final;

    void InitializeAttributes();
    void ComputeUIDs();
    void Output();
    void RegisterUID(UIND uid);

    void PrintResult(std::ostream& out) const;

public:
    explicit Spider() : INDAlgorithm(std::vector<std::string_view>{}) {
        RegisterOptions();
        MakePreprocessOptsAvailable();
    }

    void Fit(model::IDatasetStream::DataInfo const& data_info) final;

    DatasetsOrder const& GetDatasetsOrder() const final {
        return stats_.datasets_order;
    }
    INDList IndList() const;
};

}  // namespace algos
