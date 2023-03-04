#include "spider.h"

#include <iostream>
#include <set>

#include <easylogging++.h>

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/thread_number_opt.h"

namespace algos {

decltype(Spider::TempOpt) Spider::TempOpt{
        {config::names::kTemp, config::descriptions::kDSeparator}};

decltype(Spider::MemoryLimitOpt) Spider::MemoryLimitOpt{
        {config::names::kMemoryLimit, config::descriptions::kDHasHeader}};

decltype(Spider::MemoryCheckFreq) Spider::MemoryCheckFreq{
        {config::names::kMemoryCheckFrequency, config::descriptions::kDHasHeader}};

decltype(Spider::ColTypeOpt) Spider::ColTypeOpt{
        {config::names::kColType, config::descriptions::kDData}};

decltype(Spider::ValueTypeOpt) Spider::ValueTypeOpt{
        {config::names::kValueType, config::descriptions::kDData}};

void Spider::RegisterOptions() {
    RegisterOption(TempOpt.GetOption(&temp_dir_));
    RegisterOption(MemoryLimitOpt.GetOption(&ram_limit_));
    RegisterOption(config::ThreadNumberOpt.GetOption(&threads_count_));
    RegisterOption(ColTypeOpt.GetOption(&col_type_));
    RegisterOption(ValueTypeOpt.GetOption(&key_type_));
}

void Spider::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(SepOpt, HasHeaderOpt, DataOpt, TempOpt,
                                                MemoryLimitOpt, config::ThreadNumberOpt, ColTypeOpt,
                                                ValueTypeOpt));
}

template <ColTypeImpl col_type, typename... Args>
decltype(auto) CreateConcreteChunkProcessor(ColTypeImpl value, Args&&... args) {
    auto create = [&args...](auto i) -> std::unique_ptr<BaseTableProcessor> {
        constexpr auto key_type_v = static_cast<KeyTypeImpl>(static_cast<std::size_t>(i));
        using ConcreteChunkProcessor = ChunkProcessor<key_type_v, col_type>;
        return std::make_unique<ConcreteChunkProcessor>(std::forward<Args>(args)...);
    };
    return boost::mp11::mp_with_index<std::tuple_size<details::KeysTuple>>(
            static_cast<std::size_t>(value), create);
}

std::unique_ptr<BaseTableProcessor> Spider::CreateChunkProcessor(
        std::filesystem::path const& path, SpilledFilesManager& manager) const {
    BaseTableProcessor::DatasetConfig dataset{
            .path = path, .separator = separator_, .has_header = has_header_};
    auto col_type_v = static_cast<ColTypeImpl>(col_type_._to_index());

    if (col_type_ == +ColType::SET) {
        return CreateConcreteChunkProcessor<ColTypeImpl::SET>(col_type_v, manager, dataset,
                                                              ram_limit_, mem_check_frequency_);
    } else {
        return CreateConcreteChunkProcessor<ColTypeImpl::VECTOR>(col_type_v, manager, dataset,
                                                                 ram_limit_, threads_count_);
    }
}

void Spider::PreprocessData() {
    SpilledFilesManager spilled_manager{temp_dir_};

    for (const auto& path : GetPathsFromData(data_)) {
        LOG(INFO) << "Process next dataset: " << path.filename();
        auto processor = CreateChunkProcessor(path, spilled_manager);
        processor->Execute();
        state_.table_column_start_indexes.emplace_back(state_.n_cols);
        state_.n_cols += processor->GetHeaderSize();
        state_.number_of_columns.emplace_back(processor->GetHeaderSize());
    }
    state_.max_values = spilled_manager.GetMaxValues();
}

unsigned long long Spider::ExecuteInternal() {
    auto preprocess_time = std::chrono::system_clock::now();
    PreprocessData();
    auto preprocessing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - preprocess_time);

    LOG(INFO) << "Initialize attributes";
    auto init_time = std::chrono::system_clock::now();
    InitializeAttributes();
    auto initializing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - init_time);

    LOG(INFO) << "Compute UIDs";
    auto checking_time = std::chrono::system_clock::now();
    ComputeUIDs();
    auto checking = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - checking_time);

    LOG(INFO) << std::endl << "SUMMARY INFO";

    LOG(INFO) << "PreprocessingTime: " << preprocessing_time.count();
    LOG(INFO) << "InitializeTime: " << initializing_time.count();
    LOG(INFO) << "CompareTime: " << checking.count();
    Output();
    LOG(INFO) << "Deps: " << result_.size();
    return 0;
}

void Spider::RegisterUID(UID uid) {
    result_.emplace_back(std::move(uid));
}

void Spider::InitializeAttributes() {
    attrs_.reserve(state_.n_cols);
    for (std::size_t attr_id = 0; attr_id != state_.n_cols; ++attr_id) {
        auto path = SpilledFilesManager::GetResultColumnPath(attr_id);
        auto attr_ptr = new Attribute{attr_id, state_.n_cols, StrCursor{path}, state_.max_values};
        auto [attr_it, is_inserted] = attrs_.emplace(attr_id, std::move(*attr_ptr));
        if (!is_inserted) {
            throw std::runtime_error("New attribute wasn't inserted " + std::to_string(attr_id));
        }
        attr_ptr = &attr_it->second;
        if (!attr_ptr->HasFinished()) {
            attribute_queue_.emplace(attr_ptr);
        }
    }
}

void Spider::ComputeUIDs() {
    Attribute::SSet attr_ids;
    while (!attribute_queue_.empty()) {
        auto top_attribute = attribute_queue_.top();
        attribute_queue_.pop();

        attr_ids.emplace(top_attribute->GetID());
        while (!attribute_queue_.empty() &&
               top_attribute->GetCursor().GetValue() ==
                       (attribute_queue_.top()->GetCursor().GetValue())) {
            attr_ids.emplace(attribute_queue_.top()->GetID());
            attribute_queue_.pop();
        }
        for (auto attr_id : attr_ids) {
            attrs_.at(attr_id).IntersectRefs(attr_ids, attrs_);
        }
        for (auto attr_id : attr_ids) {
            auto& attr = attrs_.at(attr_id);
            if (!attr.HasFinished()) {
                attr.GetCursor().GetNext();
                attribute_queue_.emplace(&attr);
            }
        }
        attr_ids.clear();
    }
}

void Spider::Output() {
    for (const auto& [dep_id, attr] : attrs_) {
        for (auto const& ref_id : attr.GetRefs()) {
            RegisterUID({dep_id, ref_id});
        }
    }
}

void Spider::PrintResult(std::ostream& out) const {
    std::vector<std::string> columns;
    columns.reserve(state_.n_cols);
    for (std::size_t i = 0; i != paths_.size(); ++i) {
        for (std::size_t j = 0; j != state_.number_of_columns[i]; ++j) {
            std::string name = std::to_string(i) + "." + std::to_string(j);
            columns.emplace_back(name);
        }
    }
    for (UID const& uid : result_) {
        out << uid.first << "->" << uid.second;
        out << std::endl;
    }
    out << std::endl;
}

}  // namespace algos
