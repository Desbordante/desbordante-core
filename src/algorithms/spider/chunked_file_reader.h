#pragma once

#include <filesystem>
#include <fstream>
#include <functional>

namespace algos {

class ChunkedFileReader {
public:
    using ChunkPtr = char*;

private:
    std::size_t file_size_;
    std::ifstream file_;

    using ChunkData = std::unique_ptr<std::vector<char>>;
    ChunkData data_;

    ChunkPtr begin_;
    ChunkPtr end_;

    std::size_t chunk_size_;
    std::size_t specified_chunk_size_;

    std::size_t chunks_n_;
    std::size_t current_chunk_;

    void SetChunkBorders();
    ChunkPtr SpecifyChunkStart(ChunkPtr data) const;
    ChunkPtr SpecifyChunkEnd(ChunkPtr data) const;

public:
    ChunkedFileReader(std::filesystem::path const& path, std::size_t memory_limit);

    std::size_t GetChunksNumber() const {
        return chunks_n_;
    }
    std::size_t GetCurrentChunkId() const {
        return current_chunk_;
    }
    std::pair<ChunkPtr, ChunkPtr> GetCurrent() const {
        return {begin_, end_};
    }
    bool HasNext() const {
        return current_chunk_ != chunks_n_ - 1;
    }
    std::pair<ChunkPtr, ChunkPtr> GetNext();
};

}  // namespace algos
