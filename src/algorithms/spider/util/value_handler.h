#pragma once

#include <functional>
#include <ostream>
#include <string>

using PairOffset = std::pair<unsigned int, unsigned int>;

namespace algos::ind::util {

/* A utility classes for working with values of various types */

template <typename Value>
class ValueHandler {
public:
    using LessCmp = std::less<Value>;
    LessCmp less_{};
    using EqualCmp = std::equal_to<Value>;
    EqualCmp eq_{};
    using PrintFn = std::function<void(std::ostream&, Value const&)>;
    PrintFn print_ = [](std::ostream& out, Value const& v) { out << v; };
};

template <>
class ValueHandler<PairOffset> {
public:
    using BufferPtr = char*;

private:
    BufferPtr buf_begin_;

public:
    explicit ValueHandler(BufferPtr begin) : buf_begin_(begin) {}

    using LessCmp = std::function<bool(PairOffset const&, PairOffset const&)>;
    LessCmp less_ = [buffer = buf_begin_](const auto& lhs, const auto& rhs) {
        size_t lhs_len = lhs.second, rhs_len = rhs.second;
        int cmp = std::memcmp(buffer + lhs.first, buffer + rhs.first, std::min(lhs_len, rhs_len));
        return (cmp == 0 && lhs_len < rhs_len) || cmp < 0;
    };
    using EqCmp = std::function<bool(PairOffset const&, PairOffset const&)>;
    EqCmp eq_ = [buffer = buf_begin_](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return false;
        }
        return std::memcmp(buffer + lhs.first, buffer + rhs.first, lhs.second) == 0;
    };
    using PrintFn = std::function<void(std::ostream&, PairOffset const&)>;
    PrintFn print_ = [buffer = buf_begin_](std::ostream& out, PairOffset const& value) {
        auto [begin, size] = value;
        auto val_ptr = buffer + begin;
        out << std::string_view{val_ptr, size};
    };
};

}  // namespace algos::ind::util
