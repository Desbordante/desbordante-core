#pragma once

#include <cstddef>
#include <tuple>

namespace algos::hymde::utility {

template <typename... IterTypes>
class ZipIterator {
    using IterTuple = std::tuple<IterTypes...>;
    using CompType = std::tuple_element_t<1, IterTuple>;
    IterTuple iters_;

public:
    ZipIterator(IterTypes... iters) : iters_(std::forward<decltype(iters)>(iters)...) {}

    friend bool operator==(ZipIterator const& zip_it, CompType const& end_iter) {
        return std::get<1>(zip_it.iters_) == end_iter;
    }

    friend bool operator!=(ZipIterator const& zip_it, CompType const& end_iter) {
        return !(zip_it == end_iter);
    }

    ZipIterator& operator++() {
        std::apply([](auto&&... elements) { (++elements, ...); }, iters_);
        return *this;
    }

    auto operator*() {
        return std::apply(
                [](auto&&... elements) {
                    return std::tuple<typename IterTypes::reference...>(*elements...);
                },
                iters_);
        ;
    }
};

template <typename IterType1, typename IterType2, typename... IterTypes>
class Zip {
    using ZipIter = ZipIterator<IterType1, IterType2, IterTypes...>;

    ZipIter start_iter_;
    IterType2 end_iter_;

public:
    Zip(IterType2 end2, IterType1 iter1, IterType2 iter2, IterTypes... iters)
        : start_iter_(iter1, iter2, iters...), end_iter_(end2) {}

    template <typename Range1, typename Range2, typename... Ranges>
    Zip(Range1&& r1, Range2&& r2, Ranges&&... rs)
        : Zip(r2.end(), r1.begin(), r2.begin(), rs.begin()...) {}

    ZipIter begin() {
        return start_iter_;
    }

    IterType2 end() {
        return end_iter_;
    }
};

template <typename Range1, typename Range2, typename... Ranges>
Zip(Range1&& r1, Range2&& r2, Ranges&&... rs)
        -> Zip<decltype(r1.begin()), decltype(r2.begin()), decltype(rs.begin())...>;

}  // namespace algos::hymde::utility
