#pragma once

#include <cmath>
#include <malloc.h>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <boost/range/iterator_range.hpp>
#include <boost/tokenizer.hpp>
#include <easylogging++.h>

#include "chunked_file_reader.h"
#include "enums.h"
#include "model/idataset_stream.h"
#include "sorted_column_writer.h"
#include "value_handler.h"

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#define GLIBC_VERSION (__GLIBC__ * 1000 + __GLIBC_MINOR__)
#else
#define GLIBC_VERSION 0
#endif

namespace algos {

namespace details {
using KeysTuple = std::tuple<std::string_view, PairOffset>;
}

class BaseTableProcessor {
public:
    using DatasetConfig = model::IDatasetStream::DataInfo;
    using BufferPtr = ChunkedFileReader::ChunkPtr;

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
    SortedColumnWriter& writer_;
    std::vector<std::string> header_;

    std::size_t memory_limit_;
    std::size_t chunk_m_limit_;

public:
    BaseTableProcessor(SortedColumnWriter& writer, DatasetConfig dataset, std::size_t memory_limit,
                       std::size_t chunk_m_limit)
        : dataset_(std::move(dataset)),
          writer_(writer),
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
        ChunkedFileReader reader{dataset_.path, chunk_m_limit_};
        while (reader.HasNext()) {
            auto [start, end] = reader.GetNext();
            if (reader.GetCurrentChunkId() == 0) {
                start = InitHeader(start);
            }
            InitAdditionalChunkInfo(start, end);
            ProcessChunk(start, end, reader.GetChunksNumber() > 1);
        };
        writer_.MergeFiles();
    }
};

/* FIXME:
 * At the moment, we are restricted to using enum classes KeyTypeImpl and ColTypeImpl
 * as non-type template parameters due to limitations in c++17.
 * However, c++20 allows for the use of constexpr class values as non-type template
 * parameters, which will resolve this issue.
 * After upgrading to c++20 we need to use KeyType and ColType instead of 'Impl' enums
 * */

template <ind::details::KeyTypeImpl key, ind::details::ColTypeImpl col_type>
class TableProcessor : public BaseTableProcessor {
protected:
    using KeyTypeImpl = ind::details::KeyTypeImpl;
    using ColTypeImpl = ind::details::ColTypeImpl;
    using ValueType = std::tuple_element_t<(int)key, details::KeysTuple>;
    using ValueHandler = ind::details::ValueHandler<ValueType>;
    using Column = std::conditional_t<col_type == ColTypeImpl::SET,
                                      std::set<ValueType, typename ValueHandler::LessCmp>,
                                      std::vector<ValueType>>;
    using Columns = std::vector<Column>;

private:
    using EscapedList = boost::escaped_list_separator<char>;
    using Tokenizer = boost::tokenizer<EscapedList, char const*>;
    char escape_symbol = '\\';
    char quote = '\"';
    EscapedList escaped_list{escape_symbol, dataset_.separator, quote};

    static ValueHandler CreateHandler(BufferPtr start) {
        if constexpr (Is<KeyTypeImpl::PAIR>()) {
            return ValueHandler(start);
        } else {
            return ValueHandler();
        }
    }

    void ProcessChunk(BufferPtr start, BufferPtr end, bool is_chunk) override {
        auto handler = CreateHandler(start);

        ReserveColumns(handler);
        auto insert_time = std::chrono::system_clock::now();

        std::size_t length = end - start;
        auto line_begin = start, next_pos = line_begin;
        auto line_end = (char*)memchr(line_begin, '\n', length);
        std::size_t cur_attr = 0, current_row = 0;
        while (line_end != nullptr && line_end < end) {
            MemoryLimitProcess(handler, current_row++);
            Tokenizer tokens{line_begin, line_end, escaped_list};
            for (std::string const& value : tokens) {
                if (!value.empty()) {
                    bool is_quoted = (*next_pos == '\"' && *(next_pos + value.size() + 1) == '\"');
                    next_pos += is_quoted;
                    if constexpr (Is<KeyTypeImpl::STRING_VIEW>()) {
                        EmplaceValueToColumn(cur_attr, next_pos, value.size());
                    } else if constexpr (Is<KeyTypeImpl::PAIR>()) {
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

        LOG(INFO) << "Inserting: " << inserting_time.count();
        writer_.SpillColumnsToDisk(handler.print_, GetSortedColumns(handler), is_chunk);
    }

    virtual Columns& GetSortedColumns(ValueHandler const& handler) = 0;
    virtual bool IsLimitReached(std::size_t current_row) = 0;

    template <typename... Ts>
    void EmplaceValueToColumn(std::size_t attr_id, Ts... args) {
        Column& column = columns_[attr_id];
        if constexpr (Is<ColTypeImpl::VECTOR>()) {
            column.emplace_back(std::forward<Ts>(args)...);
        } else if constexpr (Is<ColTypeImpl::SET>()) {
            column.emplace(std::forward<Ts>(args)...);
        }
    }

    virtual void ReserveColumns(const ValueHandler& handler) = 0;

    void MemoryLimitProcess(ValueHandler const& handler, std::size_t current_row) {
        if (IsLimitReached(current_row)) {
            writer_.SpillColumnsToDisk(handler.print_, GetSortedColumns(handler), true);
        }
    }
    virtual std::size_t GetCurrentMemory() const = 0;

protected:
    Columns columns_;

    template <KeyTypeImpl key_value>
    static constexpr bool Is() {
        return (int)key == (int)key_value;
    }

    template <ColTypeImpl data_value>
    static constexpr bool Is() {
        return (int)col_type == (int)data_value;
    }

public:
    ~TableProcessor() override = default;

    template <typename... Args>
    explicit TableProcessor(Args&&... args) : BaseTableProcessor(std::forward<Args>(args)...) {
        if constexpr (Is<KeyTypeImpl::PAIR>()) {
            chunk_m_limit_ = std::min((double)std::numeric_limits<unsigned int>::max(),
                                      (double)chunk_m_limit_);
        }
    }
};

template <ind::details::KeyTypeImpl key>
class SetBasedTableProcessor final : public TableProcessor<key, ind::details::ColTypeImpl::SET> {
    using ColTypeImpl = ind::details::ColTypeImpl;
    using ParentType = TableProcessor<key, ColTypeImpl::SET>;
    using ValueType = typename ParentType::ValueType;
    using Columns = typename ParentType::Columns;
    using Column = typename ParentType::Column;
    using ValueHandler = typename ParentType::ValueHandler;

private:
    std::size_t memory_check_frequency_;

    Columns& GetSortedColumns(ValueHandler const&) final {
        return this->columns_;
    }
    bool IsLimitReached(std::size_t current_row) final {
        if (current_row % memory_check_frequency_ == 0) {
            return GetCurrentMemory() > (this->memory_limit_ - this->chunk_m_limit_);
        }
        return false;
    }

    void ReserveColumns(const ValueHandler& handler) final {
        this->columns_.assign(this->GetHeaderSize(), Column{handler.less_});
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
    SetBasedTableProcessor(SortedColumnWriter& writer, BaseTableProcessor::DatasetConfig config,
                           std::size_t memory_limit, std::size_t mem_check_frequency)
        : ParentType(writer, std::move(config), memory_limit, memory_limit / 3 * 2),
          memory_check_frequency_(mem_check_frequency) {}

    ~SetBasedTableProcessor() final = default;
};

template <ind::details::KeyTypeImpl key>
class VectorBasedTableProcessor final
    : public TableProcessor<key, ind::details::ColTypeImpl::VECTOR> {
    using ColTypeImpl = ind::details::ColTypeImpl;
    using ParentType = TableProcessor<key, ColTypeImpl::VECTOR>;
    using BufferPtr = typename ParentType::BufferPtr;
    using ValueType = typename ParentType::ValueType;
    using Columns = typename ParentType::Columns;
    using ValueHandler = typename ParentType::ValueHandler;

    std::size_t threads_count_;
    std::size_t rows_limit_;
    std::size_t rows_count_;

    std::size_t CalculateRowsLimit(std::size_t rows_number) const {
        std::size_t row_memory = this->GetHeaderSize() * sizeof(ValueType);
        std::size_t needed_memory = rows_number * row_memory;
        std::size_t data_memory_limit = this->memory_limit_ - this->chunk_m_limit_;

        std::size_t rows_limit;
        if (needed_memory > data_memory_limit) {
            std::size_t times = std::ceil((double)needed_memory / (double)data_memory_limit);
            rows_limit = std::ceil((double)rows_number / (double)times);
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

        LOG(INFO) << "Counting time: " << counting_time.count();

        rows_limit_ = CalculateRowsLimit(rows_count_);
    }

    void ReserveColumns(ValueHandler const&) final {
        this->columns_.assign(this->GetHeaderSize(), {});
        for (auto& col : this->columns_) {
            col.reserve(rows_limit_);
        }
    }

    void Sort(ValueHandler const& handler) {
        auto sort_time = std::chrono::system_clock::now();
        std::vector<std::thread> threads;
        for (std::size_t j = 0; j < this->GetHeaderSize(); ++j) {
            /* TODO: At this point it is preferable to use a thread pool */
            threads.emplace_back([&col = this->columns_[j], &handler]() {
                std::sort(col.begin(), col.end(), handler.less_);
                col.erase(unique(col.begin(), col.end(), handler.eq_), col.end());
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
        LOG(INFO) << "Sorting: " << sorting_time.count();
    }

    Columns& GetSortedColumns(ValueHandler const& handler) final {
        Sort(handler);
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
    VectorBasedTableProcessor(SortedColumnWriter& writer,
                              const BaseTableProcessor::DatasetConfig& config,
                              std::size_t memory_limit, std::size_t threads_count)
        : ParentType(writer, config, memory_limit, memory_limit / 2),
          threads_count_(threads_count) {}

    ~VectorBasedTableProcessor() final = default;
};

template <ind::details::KeyTypeImpl key, ind::details::ColTypeImpl col>
using ChunkProcessor =
        std::conditional_t<ind::details::ColTypeImpl::SET == col, SetBasedTableProcessor<key>,
                           VectorBasedTableProcessor<key>>;

}  // namespace algos
