#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace util {

unsigned LevenshteinDistance(std::string_view l, std::string_view r);

namespace {
std::size_t LevenshteinDistanceMain(unsigned* p, unsigned* d, std::size_t max_dist, auto* l,
                                    std::size_t l_size, auto* r, std::size_t r_size);
}  // namespace

std::size_t LevenshteinDistance(std::string const* l_ptr, std::string const* r_ptr, unsigned* p,
                                unsigned* d, std::size_t max_dist, std::size_t fail_value) noexcept;

}  // namespace util
