#pragma once

#include <functional>
#include <string>

using PairOffset = std::pair<unsigned int, unsigned int>;

namespace algos::details {

template <typename Value>
class Comparator {
public:
    using LessCmp = std::less<Value>;
    LessCmp less_{};
    using EqualCmp = std::equal_to<Value>;
    EqualCmp eq_{};
    explicit Comparator() = default;
    using ToPrintable = std::function<Value(Value const&)>;
    ToPrintable to_print_ = [](Value const& x) { return x; };
};

template <>
class Comparator<PairOffset> {
public:
    using BufferPtr = char*;

private:
    BufferPtr buf_begin;

public:
    explicit Comparator(BufferPtr begin) : buf_begin(begin) {}

    using LessCmp = std::function<bool(PairOffset const&, PairOffset const&)>;
    LessCmp less_ = [buffer = buf_begin](const auto& lhs, const auto& rhs) {
        size_t lhs_len = lhs.second, rhs_len = rhs.second;
        int cmp = std::memcmp(buffer + lhs.first, buffer + rhs.first, std::min(lhs_len, rhs_len));
        return (cmp == 0 && lhs_len < rhs_len) || cmp < 0;
    };
    using EqCmp = std::function<bool(PairOffset const&, PairOffset const&)>;
    EqCmp eq_ = [buffer = buf_begin](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return false;
        }
        return std::memcmp(buffer + lhs.first, buffer + rhs.first, lhs.second) == 0;
    };
    using ToPrintable = std::function<std::string_view(PairOffset const&)>;
    ToPrintable to_print_ = [buffer = buf_begin](PairOffset const& value) {
        auto [begin, size] = value;
        auto val_ptr = buffer + begin;
        return std::string_view{val_ptr, size};
    };
};
}  // namespace algos::details
