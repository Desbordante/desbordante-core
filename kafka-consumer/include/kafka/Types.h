#pragma once

#include "kafka/Project.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>


// Use `boost::optional` for C++14, which doesn't support `std::optional`
#if __cplusplus >= 201703L
#include <optional>
template<class T>
using Optional = std::optional<T>;
#else
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
template<class T>
using Optional = boost::optional<T>;
#endif


namespace KAFKA_API {

// Which is similar with `boost::const_buffer` (thus avoid the dependency towards `boost`)
class ConstBuffer
{
public:
    explicit ConstBuffer(const void* data = nullptr, std::size_t size = 0): _data(data), _size(size) {}
    const void* data()     const { return _data; }
    std::size_t size()     const { return _size; }
    std::string toString() const
    {
        if (_size == 0) return _data ? "[empty]" : "[NULL]";

        std::ostringstream oss;

        auto printChar = [&oss](const unsigned char c) {
            if (std::isprint(c)) {
                oss << c;
            } else {
                oss << "[0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c) << "]";
            }
        };
        const auto* beg = static_cast<const unsigned char*>(_data);
        std::for_each(beg, beg + _size, printChar);

        return oss.str();
    }
private:
    const void* _data;
    std::size_t _size;
};

/**
 * Topic name.
 */
using Topic     = std::string;

/**
 * Partition number.
 */
using Partition = std::int32_t;

/**
 * Record offset.
 */
using Offset    = std::int64_t;

/**
 * Record key.
 */
using Key       = ConstBuffer;
using KeySize   = std::size_t;

/**
 * Null Key.
 */
#if __cplusplus >= 201703L
const inline Key NullKey = Key{};
#else
const static Key NullKey = Key{};
#endif

/**
 * Record value.
 */
using Value     = ConstBuffer;
using ValueSize = std::size_t;

/**
 * Null Value.
 */
#if __cplusplus >= 201703L
const inline Value NullValue = Value{};
#else
const static Value NullValue = Value{};
#endif

/**
 * Topic set.
 */
using Topics                = std::set<Topic>;

/**
 * Topic Partition pair.
 */
using TopicPartition        = std::pair<Topic, Partition>;

/**
 * TopicPartition set.
 */
using TopicPartitions       = std::set<TopicPartition>;

/**
 * Topic/Partition/Offset tuple
 */
using TopicPartitionOffset  = std::tuple<Topic, Partition, Offset>;

/**
 * TopicPartition to Offset map.
 */
using TopicPartitionOffsets = std::map<TopicPartition, Offset>;


/**
 * Obtains explanatory string for Topics.
 */
inline std::string toString(const Topics& topics)
{
    std::string ret;
    std::for_each(topics.cbegin(), topics.cend(),
                  [&ret](const auto& topic) {
                      ret.append(ret.empty() ? "" : ",").append(topic);
                  });
    return ret;
}

/**
 * Obtains explanatory string for TopicPartition.
 */
inline std::string toString(const TopicPartition& tp)
{
    return tp.first + std::string("-") + std::to_string(tp.second);
}

/**
 * Obtains explanatory string for TopicPartitions.
 */
inline std::string toString(const TopicPartitions& tps)
{
    std::string ret;
    std::for_each(tps.cbegin(), tps.cend(),
                  [&ret](const auto& tp) {
                      ret.append((ret.empty() ? "" : ",") + tp.first + "-" + std::to_string(tp.second));
                  });
    return ret;
}

/**
 * Obtains explanatory string for TopicPartitionOffset.
 */
inline std::string toString(const TopicPartitionOffset& tpo)
{
    return std::get<0>(tpo) + "-" + std::to_string(std::get<1>(tpo)) + ":" + std::to_string(std::get<2>(tpo));
}

/**
 * Obtains explanatory string for TopicPartitionOffsets.
 */
inline std::string toString(const TopicPartitionOffsets& tpos)
{
    std::string ret;
    std::for_each(tpos.cbegin(), tpos.cend(),
                  [&ret](const auto& tp_o) {
                      const TopicPartition& tp = tp_o.first;
                      const Offset& o  = tp_o.second;
                      ret.append((ret.empty() ? "" : ",") + tp.first + "-" + std::to_string(tp.second) + ":" + std::to_string(o));
                  });
    return ret;
}

} // end of KAFKA_API

