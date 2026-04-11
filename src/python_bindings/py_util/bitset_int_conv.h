#pragma once

#include <bit>
#include <cstddef>
#include <limits>

#include <Python.h>
#include <boost/dynamic_bitset.hpp>
#include <pybind11/pybind11.h>

#include "core/model/index.h"
#include "core/util/bitset_utils.h"

namespace python_bindings {
inline static pybind11::int_ BitsetToInt(boost::dynamic_bitset<> const& bitset) {
    pybind11::int_ py_int{0};
    pybind11::int_ const py_one = pybind11::int_{1};
    util::ForEachIndex(bitset, [&](model::Index i) { py_int |= py_one << pybind11::int_{i}; });
    return py_int;
}

inline static boost::dynamic_bitset<> IntToBitset(pybind11::int_ py_int, std::size_t const size) {
    // TODO: use PyLongExport.
    boost::dynamic_bitset<> bitset{size};
    if (py_int < pybind11::int_{0}) {
        throw pybind11::value_error("Unexpected negative number as bit storage: " +
                                    static_cast<std::string>(pybind11::str(py_int)));
    }
    static constexpr std::size_t kBlockDigits = std::numeric_limits<unsigned long long>::digits;
    pybind11::int_ const block_digits_py = pybind11::int_{kBlockDigits};
    pybind11::int_ bits = py_int;
    std::size_t block_offset = 0;
    while (bits.cast<bool>()) {
        unsigned long long block = PyLong_AsUnsignedLongLongMask(bits.ptr());
        if (block == (unsigned long long)-1 && PyErr_Occurred())
            throw pybind11::error_already_set();
        while (block != 0) {
            std::size_t const block_bit_index = std::countr_zero(block);
            std::size_t const bit_index = block_offset + block_bit_index;
            if (bit_index >= size) {
                throw pybind11::value_error(
                        "Number " + static_cast<std::string>(pybind11::str(py_int)) +
                        " stores more bits than expected (" + std::to_string(size) + ")");
            }
            bitset.set(bit_index);
            block ^= (1ULL << block_bit_index);
        }
        bits >>= block_digits_py;
        block_offset += kBlockDigits;
    }
    return bitset;
}
}  // namespace python_bindings
