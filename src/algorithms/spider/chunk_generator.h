#include <algorithm>
#include <cmath>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <utility>

namespace algos {

class ChunkGenerator {
public:
    using CharPtr = char*;

private:
    std::size_t file_size_;
    int fd_;

    using ChunkData = std::unique_ptr<char, std::function<void(CharPtr)>>;
    ChunkData data_;

    // Specified values
    CharPtr begin_;
    CharPtr end_;

    std::size_t chunk_size_;
    std::size_t specified_chunk_size_;

    std::size_t chunks_n_;
    std::size_t current_chunk_;

    void SetChunkBorders();
    CharPtr SpecifyChunkStart(CharPtr data) const;
    CharPtr SpecifyChunkEnd(CharPtr data) const;

public:
    ChunkGenerator(std::filesystem::path const& path, std::size_t memory_limit);
    std::size_t GetChunksNumber() const {
        return chunks_n_;
    }
    std::size_t GetCurrentChunkId() const {
        return current_chunk_;
    }
    std::pair<CharPtr, CharPtr> GetCurrent() const {
        return {begin_, end_};
    }
    bool HasNext() const {
        return current_chunk_ != chunks_n_ - 1;
    }
    std::pair<CharPtr, CharPtr> GetNext();
};

}  // namespace algos