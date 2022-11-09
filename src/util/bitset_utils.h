#include <boost/dynamic_bitset.hpp>

namespace util {

template <typename ForwardIt>
boost::dynamic_bitset<> IndicesToBitset(ForwardIt begin, ForwardIt end, size_t num_columns) {
    boost::dynamic_bitset<> bitset(num_columns);
    for (auto it = begin; it != end; ++it) {
        bitset.set(*it);
    }
    return bitset;
}

template <typename Container>
boost::dynamic_bitset<> IndicesToBitset(Container const& indices, size_t num_columns) {
    return IndicesToBitset(indices.cbegin(), indices.cend(), num_columns);
}

}  // namespace util
