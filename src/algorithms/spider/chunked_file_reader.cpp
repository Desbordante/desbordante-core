#include "chunked_file_reader.h"

#include <cmath>
#include <filesystem>
#include <string>

#include <easylogging++.h>

namespace algos {

static std::size_t kOverlapSize = 4 << 10;

using ChunkPtr = ChunkedFileReader::ChunkPtr;

void ChunkedFileReader::SetChunkBorders() {
    begin_ = SpecifyChunkStart(data_->data());
    end_ = SpecifyChunkEnd(data_->data() + data_->size());
}

ChunkPtr ChunkedFileReader::SpecifyChunkStart(ChunkPtr data) const {
    if (current_chunk_ == 0) {
        return data;
    }
    while (*(data++) != '\n')
        ;
    return data;
}

ChunkPtr ChunkedFileReader::SpecifyChunkEnd(ChunkPtr data) const {
    if (current_chunk_ == chunks_n_ - 1) {
        return data;
    }
    while (*(--data) != '\n')
        ;
    return data;
}

ChunkedFileReader::ChunkedFileReader(std::filesystem::path const& path, std::size_t memory_limit)
    : file_size_(std::filesystem::file_size(path)),
      file_(path, std::ios::binary),
      begin_(nullptr),
      end_(nullptr) {
    if (file_.fail()) {
        throw std::runtime_error("Failed to open file " + std::string(path));
    }
    chunks_n_ = std::ceil((double)file_size_ / (double)memory_limit);
    chunk_size_ = (file_size_ / chunks_n_) & ~(kOverlapSize - 1);
    specified_chunk_size_ = std::min(chunk_size_ + kOverlapSize, file_size_);
    current_chunk_ = -1;
}

std::pair<ChunkPtr, ChunkPtr> ChunkedFileReader::GetNext() {
    if (!HasNext()) {
        throw std::runtime_error("Invalid function call, no more chunks");
    }
    auto page_size = sysconf(_SC_PAGE_SIZE);
    current_chunk_++;
    auto offset = current_chunk_ * chunk_size_;
    if (current_chunk_ == chunks_n_ - 1) {
        specified_chunk_size_ = file_size_ - offset;
        if (current_chunk_ != 0) {
            data_->resize(specified_chunk_size_);
        }
    }
    if (chunks_n_ != 1) {
        LOG(INFO) << "Chunk: " << current_chunk_;
    }

    if (current_chunk_ == 0) {
        data_ = std::make_unique<std::vector<char>>(specified_chunk_size_);
    } else {
        file_.seekg(-page_size, std::ios::cur);
    }

    file_.read(data_->data(), data_->size());
    SetChunkBorders();
    return GetCurrent();
}

}  // namespace algos
