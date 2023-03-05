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
#include "util/value_handler.h"

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#define GLIBC_VERSION (__GLIBC__ * 1000 + __GLIBC_MINOR__)
#else
#define GLIBC_VERSION 0
#endif

namespace algos::ind::preproc {

namespace details {
using KeysTuple = std::tuple<std::string_view, PairOffset>;
}

class BaseTableProcessor {
public:
    using DataConfig = model::IDatasetStream::DataInfo;
    using BufferPtr = ChunkedFileReader::ChunkPtr;

private:
    virtual void ProcessChunk(BufferPtr begin, BufferPtr end, bool is_chunk) = 0;

    BufferPtr InitHeader(BufferPtr start);

    virtual void InitAdditionalChunkInfo(BufferPtr, BufferPtr) {
        /* by default do nothing */
    }

protected:
    using EscapedList = boost::escaped_list_separator<char>;
    using Tokenizer = boost::tokenizer<EscapedList, char const*>;

    DataConfig dataset_;
    char escape_symbol_ = '\\';
    char quote_ = '\"';
    EscapedList escaped_list_{escape_symbol_, dataset_.separator, quote_};

    SortedColumnWriter& writer_;
    std::vector<std::string> header_;

    std::size_t memory_limit_;
    std::size_t chunk_m_limit_;

public:
    BaseTableProcessor(SortedColumnWriter& writer, DataConfig dataset, std::size_t memory_limit,
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
    void Execute();
};

}  // namespace algos::ind::preproc
