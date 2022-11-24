#pragma once

#include <iterator>
#include <optional>
#include <vector>

#include <boost/align/aligned_allocator.hpp>

namespace algos::faida {

class IRowIterator {
public:
    using AlignedVector = std::vector<size_t, boost::alignment::aligned_allocator<size_t, 32>>;
    using Block = std::vector<std::optional<AlignedVector>>;

    virtual bool HasNextBlock() = 0;
    virtual size_t GetBlockSize() const = 0;
    virtual Block const& GetNextBlock() = 0;

    virtual ~IRowIterator() = default;
};

}  // namespace algos::faida
