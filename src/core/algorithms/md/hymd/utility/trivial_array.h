#pragma once

#include <cstddef>
#include <memory>
#include <ranges>
#include <type_traits>

#include "algorithms/md/hymd/utility/make_unique_for_overwrite.h"
#include "algorithms/md/hymd/utility/zip.h"
#include "util/py_tuple_hash.h"

namespace algos::hymd::utility {
template <typename T>
class PointerArrayBackInserter {
    T* cur_ptr_;

public:
    PointerArrayBackInserter(T* initial) : cur_ptr_(initial) {}

    void PushBack(T element) {
        *cur_ptr_++ = element;
    }
};

template <typename T>
class TrivialArray {
    static_assert(std::is_trivially_destructible_v<T>);

    std::unique_ptr<T[]> data_;

public:
    class Hasher {
        std::size_t n_;

    public:
        Hasher(std::size_t n) : n_(n) {}

        std::size_t operator()(TrivialArray const& arr) const noexcept {
            util::PyTupleHash hasher{n_};
            for (T const& el : std::span{arr.data_.get(), n_}) {
                hasher.AddValue(el);
            }
            return hasher.GetResult();
        }
    };

    class ArrEqual {
        std::size_t n_;

    public:
        ArrEqual(std::size_t n) : n_(n) {}

        bool operator()(TrivialArray const& arr1, TrivialArray const& arr2) const noexcept {
            for (auto [el1, el2] : utility::Zip(arr1.GetSpan(n_), arr2.GetSpan(n_))) {
                if (el1 != el2) return false;
            }
            return true;
        }
    };

    TrivialArray(std::size_t n) : data_(utility::MakeUniqueForOverwrite<T[]>(n)) {}

    TrivialArray(std::span<T> s) : data_(utility::MakeUniqueForOverwrite<T[]>(s.size())) {
        std::ranges::copy(s, data_.get());
    }

    std::span<T> GetSpan(std::size_t size) noexcept {
        return {data_.get(), size};
    }

    std::span<T const> GetSpan(std::size_t size) const noexcept {
        return {data_.get(), size};
    }

    T const& operator[](std::size_t i) const noexcept {
        return data_[i];
    }

    T& operator[](std::size_t i) noexcept {
        return data_[i];
    }

    PointerArrayBackInserter<T> GetFiller() noexcept {
        PointerArrayBackInserter inserter(data_.get());
        return inserter;
    }
};
}  // namespace algos::hymd::utility
