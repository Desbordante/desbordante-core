#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <enum.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include <boost/range/iterator_range.hpp>
#include <boost/tokenizer.hpp>

#include "adapter.h"
#include "chunk_generator.h"
#include "spilled_files_manager.h"

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#define GLIBC_VERSION (__GLIBC__ * 1000 + __GLIBC_MINOR__)
#else
#define GLIBC_VERSION 0
#endif

namespace algos {

BETTER_ENUM(KeyType, char, STRING_VIEW, PAIR)
BETTER_ENUM(ColType, char, SET, VECTOR)

enum class KeyTypeImpl { STRING_VIEW, PAIR };
enum class ColTypeImpl { SET, VECTOR };

namespace details {
using KeysTuple = std::tuple<std::string_view, PairOffset>;
}

class BaseTableProcessor {
public:
    using BufferPtr = ChunkGenerator::CharPtr;

    struct DatasetConfig {
        std::filesystem::path path;
        char separator;
        bool has_header;
    };

private:
    virtual void ProcessChunk(BufferPtr begin, BufferPtr end, bool is_chunk) = 0;

    BufferPtr InitHeader(BufferPtr buffer) {
        char const* pos = buffer;
        while (*pos != '\0' && *pos != '\n') {
            char const* next_pos = pos;
            while (*next_pos != '\0' && *next_pos != dataset_.separator && *next_pos != '\n') {
                next_pos++;
            }
            if (dataset_.has_header) {
                header_.emplace_back(pos, next_pos - pos);
            } else {
                header_.emplace_back(std::to_string(GetHeaderSize()));
            }
            pos = next_pos + (*next_pos == dataset_.separator);
        }
        if (dataset_.has_header) {
            buffer = (BufferPtr)pos;
        }
        return dataset_.has_header ? (BufferPtr)pos : buffer;
    }

    virtual void InitAdditionalChunkInfo(BufferPtr, BufferPtr) {}

protected:
    DatasetConfig dataset_;
    SpilledFilesManager& spilled_manager_;
    std::vector<std::string> header_;

    std::size_t memory_limit_;
    std::size_t chunk_m_limit_;

public:
    BaseTableProcessor(SpilledFilesManager& spilled_manager, DatasetConfig dataset,
                       std::size_t memory_limit, std::size_t chunk_m_limit)
        : dataset_(std::move(dataset)),
          spilled_manager_(spilled_manager),
          memory_limit_(memory_limit),
          chunk_m_limit_(chunk_m_limit) {}
    virtual ~BaseTableProcessor() = default;

    std::size_t GetHeaderSize() const {
        return header_.size();
    }
    std::vector<std::string> const& GetHeader() const {
        return header_;
    }
    void Execute() {
        ChunkGenerator chunk_generator_{dataset_.path, chunk_m_limit_};
        while (chunk_generator_.HasNext()) {
            auto [start, end] = chunk_generator_.GetNext();
            if (chunk_generator_.GetCurrentChunkId() == 0) {
                start = InitHeader(start);
            }
            InitAdditionalChunkInfo(start, end);
            ProcessChunk(start, end, chunk_generator_.GetChunksNumber() > 1);
        };
        spilled_manager_.MergeFiles();
    }
};

template <KeyTypeImpl key, ColTypeImpl col_type>
class TableProcessor : public BaseTableProcessor {
protected:
    using ValueType = std::tuple_element_t<(int)key, details::KeysTuple>;
    using Adapter = details::Comparator<ValueType>;
    using Column = std::conditional_t<col_type == ColTypeImpl::SET,
                                      std::set<ValueType, typename Adapter::LessCmp>,
                                      std::vector<ValueType>>;
    using Columns = std::vector<Column>;

private:
    using EscapedList = boost::escaped_list_separator<char>;
    using Tokenizer = boost::tokenizer<EscapedList, char const*>;
    char escape_symbol = '\\';
    char quote = '\"';
    EscapedList escaped_list{escape_symbol, dataset_.separator, quote};

    static Adapter CreateAdapter(BufferPtr start) {
        if constexpr (is<KeyTypeImpl::PAIR>()) {
            return Adapter(start);
        } else {
            return Adapter();
        }
    }

    void ProcessChunk(BufferPtr start, BufferPtr end, bool is_chunk) override {
        auto adapter = CreateAdapter(start);

        ReserveColumns(adapter);
        auto insert_time = std::chrono::system_clock::now();

        std::size_t length = end - start;
        auto line_begin = start, next_pos = line_begin;
        auto line_end = (char*)memchr(line_begin, '\n', length);
        std::size_t cur_attr = 0, current_row = 0;
        while (line_end != nullptr && line_end < end) {
            MemoryLimitProcess(adapter, current_row++);
            Tokenizer tokens{line_begin, line_end, escaped_list};
            for (std::string const& value : tokens) {
                if (!value.empty()) {
                    bool is_quoted = (*next_pos == '\"' && *(next_pos + value.size() + 1) == '\"');
                    next_pos += is_quoted;
                    if constexpr (is<KeyTypeImpl::STRING_VIEW>()) {
                        EmplaceValueToColumn(cur_attr, next_pos, value.size());
                    } else if constexpr (is<KeyTypeImpl::PAIR>()) {
                        EmplaceValueToColumn(cur_attr, next_pos - start, value.size());
                    }
                    next_pos += value.size() + is_quoted;
                }
                next_pos++;
                cur_attr++;
            }
            cur_attr = 0;

            line_begin = line_end + 1;
            next_pos = line_begin;
            length = end - line_begin;
            line_end = (char*)memchr(line_begin, '\n', length);
        }
        auto inserting_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - insert_time);

        std::cout << "Inserting: " << inserting_time.count() << std::endl;
        spilled_manager_.SpillColumnsToDisk(adapter.to_print_, GetSortedColumns(adapter), is_chunk);
    }

    virtual Columns& GetSortedColumns(Adapter const& adapter) = 0;
    virtual bool IsLimitReached(std::size_t current_row) = 0;

    template <typename... Ts>
    void EmplaceValueToColumn(std::size_t attr_id, Ts... args) {
        Column& column = columns_[attr_id];
        if constexpr (isCol<ColTypeImpl::VECTOR>()) {
            column.emplace_back(std::forward<Ts>(args)...);
        } else if constexpr (isCol<ColTypeImpl::SET>()) {
            column.emplace(std::forward<Ts>(args)...);
        }
    }

    virtual void ReserveColumns(const Adapter& adapter) = 0;

    void MemoryLimitProcess(Adapter const& adapter, std::size_t current_row) {
        if (IsLimitReached(current_row)) {
            spilled_manager_.SpillColumnsToDisk(adapter.to_print_, GetSortedColumns(adapter), true);
        }
    }
    virtual std::size_t GetCurrentMemory() const = 0;

protected:
    Columns columns_;

    template <KeyTypeImpl key_value>
    static constexpr bool is() {
        return (int)key == (int)key_value;
    }

    template <ColTypeImpl data_value>
    static constexpr bool isCol() {
        return (int)col_type == (int)data_value;
    }

public:
    ~TableProcessor() override = default;

    template <typename... Args>
    explicit TableProcessor(Args&&... args) : BaseTableProcessor(std::forward<Args>(args)...) {
        if constexpr (is<KeyTypeImpl::PAIR>()) {
            chunk_m_limit_ = std::min((double)std::numeric_limits<unsigned int>::max(),
                                      (double)chunk_m_limit_);
        }
    }
};

template <KeyTypeImpl key>
class SetBasedTableProcessor final : public TableProcessor<key, ColTypeImpl::SET> {
    using ParentType = TableProcessor<key, ColTypeImpl::SET>;
    using ValueType = typename ParentType::ValueType;
    using Columns = typename ParentType::Columns;
    using Column = typename ParentType::Column;
    using Adapter = typename ParentType::Adapter;

private:
    std::size_t memory_check_frequency_;

    Columns& GetSortedColumns(Adapter const&) final {
        return this->columns_;
    }
    bool IsLimitReached(std::size_t current_row) final {
        if (current_row % memory_check_frequency_ == 0) {
            return GetCurrentMemory() > (this->memory_limit_ - this->chunk_m_limit_);
        }
        return false;
    }

    void ReserveColumns(const Adapter& adapter) final {
        this->columns_.assign(this->GetHeaderSize(), Column{adapter.less_});
    }

    std::size_t GetCurrentMemory() const final {
#if GLIBC_VERSION >= 2033
        return mallinfo2().uordblks;
#else
        double coeff = 200.0 / 167;
        std::size_t estimation = sizeof(T);
        for (auto const& column : columns_) {
            estimation += column.size() * sizeof(std::_Rb_tree_node<typename T::key_type>);
        }
        return (std::size_t)(coeff * (double)estimation);
#endif
    }

public:
    SetBasedTableProcessor(SpilledFilesManager& spilled_manager,
                           BaseTableProcessor::DatasetConfig config, std::size_t memory_limit,
                           std::size_t mem_check_frequency)
        : ParentType(spilled_manager, std::move(config), memory_limit, memory_limit / 3 * 2),
          memory_check_frequency_(mem_check_frequency) {}

    ~SetBasedTableProcessor() final = default;
};

template <KeyTypeImpl key>
class VectorBasedTableProcessor final : public TableProcessor<key, ColTypeImpl::VECTOR> {
    using ParentType = TableProcessor<key, ColTypeImpl::VECTOR>;
    using BufferPtr = typename ParentType::BufferPtr;
    using ValueType = typename ParentType::ValueType;
    using Columns = typename ParentType::Columns;
    using Adapter = typename ParentType::Adapter;

    std::size_t threads_count_;
    std::size_t rows_limit_;
    std::size_t rows_count_;

    std::size_t GetRowsLimit(std::size_t rows_number) const {
        std::size_t row_memory = this->GetHeaderSize() * sizeof(ValueType);
        std::size_t needed_memory = rows_number * row_memory;
        std::size_t data_memory_limit = this->memory_limit_ - this->chunk_m_limit_;

        std::size_t rows_limit;
        if (needed_memory > data_memory_limit) {
            std::size_t times = std::ceil((double)needed_memory / (double)data_memory_limit);
            rows_limit = std::ceil((double)rows_number / (double)times);
            std::cout << (needed_memory >> 20) << " > " << (data_memory_limit >> 20) << " ["
                      << times << "]\n";
        } else {
            rows_limit = rows_number;
        }
        return rows_limit;
    }

    void InitAdditionalChunkInfo(BufferPtr start, BufferPtr end) final {
        auto count_time = std::chrono::system_clock::now();

        rows_count_ = std::count(start, end, '\n');

        auto counting_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - count_time);

        std::cout << "Counting time: " << counting_time.count() << std::endl;

        rows_limit_ = GetRowsLimit(rows_count_);
    }

    void ReserveColumns(Adapter const&) final {
        this->columns_.assign(this->GetHeaderSize(), {});
        for (auto& col : this->columns_) {
            col.reserve(rows_limit_);
        }
    }

    void Sort(Adapter const& adapter) {
        auto sort_time = std::chrono::system_clock::now();
        std::vector<std::thread> threads;
        for (std::size_t j = 0; j < this->GetHeaderSize(); ++j) {
            threads.emplace_back([&col = this->columns_[j], &adapter]() {
                std::sort(col.begin(), col.end(), adapter.less_);
                col.erase(unique(col.begin(), col.end(), adapter.eq_), col.end());
            });
            if ((j != 0 && j % threads_count_ == 0) || j == this->GetHeaderSize() - 1) {
                for (auto& th : threads) {
                    th.join();
                }
                threads.clear();
            }
        }
        auto sorting_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - sort_time);
        std::cout << "Sorting: " << sorting_time.count() << std::endl;
    }

    Columns& GetSortedColumns(Adapter const& adapter) final {
        Sort(adapter);
        return this->columns_;
    }

    bool IsLimitReached(std::size_t current_row) final {
        return rows_count_ != rows_limit_ && current_row % rows_limit_ == 0;
    }

    std::size_t GetCurrentMemory() const final {
        std::size_t row_memory = this->GetHeaderSize() * sizeof(ValueType);
        return row_memory * this->columns_.front().capacity();
    }

public:
    VectorBasedTableProcessor(SpilledFilesManager& spilled_manager,
                              const BaseTableProcessor::DatasetConfig& config,
                              std::size_t memory_limit, std::size_t threads_count)
        : ParentType(spilled_manager, config, memory_limit, memory_limit / 2),
          threads_count_(threads_count) {}

    ~VectorBasedTableProcessor() final = default;
};

template <KeyTypeImpl key, ColTypeImpl col>
using ChunkProcessor = std::conditional_t<ColTypeImpl::SET == col, SetBasedTableProcessor<key>,
                                          VectorBasedTableProcessor<key>>;

}  // namespace algos
