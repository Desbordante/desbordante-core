#include "base_table_processor.h"

namespace algos::ind::preproc {

BaseTableProcessor::BufferPtr BaseTableProcessor::InitHeader(BufferPtr start) {
    auto line_end = (BufferPtr)strchr(start, '\n');
    Tokenizer tokens{start, line_end, escaped_list_};

    for (std::string const& value : tokens) {
        auto const& attr_name = dataset_.has_header ? value : std::to_string(GetHeaderSize());
        header_.emplace_back(attr_name);
    }
    return dataset_.has_header ? start : line_end + 1;
}

void BaseTableProcessor::Execute() {
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

}  // namespace algos::ind::preproc
