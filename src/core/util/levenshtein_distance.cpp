#include "core/util/levenshtein_distance.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

#include "core/model/index.h"
#include "core/util/desbordante_assume.h"

namespace util {

/* Levenshtein distance computation algorithm taken from
 * https://en.wikipedia.org/wiki/Levenshtein_distance
 */
unsigned LevenshteinDistance(std::string_view l, std::string_view r) {
    size_t r_size = r.size();
    std::vector<unsigned> v0(r_size + 1);
    std::vector<unsigned> v1(r_size + 1);

    for (unsigned i = 0; i != r_size + 1; ++i) {
        v0[i] = i;
    }

    for (unsigned i = 0; i != l.size(); ++i) {
        v1[0] = i + 1;

        for (unsigned j = 0; j != r.size(); ++j) {
            assert(j + 1 < v0.size());
            unsigned del_cost = v0[j + 1] + 1;
            unsigned insert_cost = v1[j] + 1;
            unsigned substitution_cost;
            if (l[i] == r[j]) {
                substitution_cost = v0[j];
            } else {
                substitution_cost = v0[j] + 1;
            }

            v1[j + 1] = std::min({del_cost, insert_cost, substitution_cost});
        }

        std::swap(v0, v1);
    }

    return v0.back();
}

namespace {

std::size_t LevenshteinDistanceMain(unsigned* p, unsigned* d, std::size_t max_dist, auto* l,
                                    std::size_t l_size, auto* r, std::size_t r_size) {
    if (l_size <= max_dist) {
        std::iota(p, p + l_size + 1, 0);
    } else {
        auto iota_end = p + max_dist + 1;
        std::iota(p, iota_end, 0);
        std::fill(iota_end, p + l_size + 1, -1);
    }
    std::fill(d, d + l_size + 1, -1);
    DESBORDANTE_ASSUME(l_size <= r_size);
    DESBORDANTE_ASSUME(l_size != 0);
    model::Index i = 0;
    auto get_zero = [](model::Index) { return 0; };
    auto get_l_size = [l_size](model::Index) { return l_size; };
    auto get_n0_min = [max_dist](model::Index i) { return i - max_dist; };
    auto set_max = [](auto* d, model::Index min) { d[min] = -1; };
    auto ignore = [](auto*, model::Index) {};
    auto do_loop = [&r, &l](auto* p, auto* d, model::Index i, auto get_min, auto get_max,
                            auto max_func) {
        auto tj = r[i];
        *d = i + 1;

        model::Index const min = get_min(i);
        model::Index const max = get_max(i);

        max_func(d, min);
        for (model::Index j = min; j != max;) {
            unsigned value = p[j];
            if (l[j] == tj) {
                ++j;
            } else {
                unsigned const insert_cost = d[j];
                ++j;
                value = 1 + std::min({insert_cost, value, p[j]});
            }
            d[j] = value;
        }
    };
    if (r_size <= max_dist) {
        if (r_size % 2 == 0) {
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                ++i;
                do_loop(d, p, i, get_zero, get_l_size, ignore);
            }
            return p[l_size];
        } else {
            --r_size;
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                ++i;
                do_loop(d, p, i, get_zero, get_l_size, ignore);
            }
            do_loop(p, d, r_size, get_zero, get_l_size, ignore);
            return d[l_size];
        }
    } else {
        std::size_t const max_dist_inc = max_dist + 1;
        auto get_nl_max = [max_dist_inc](model::Index i) { return i + max_dist_inc; };
        DESBORDANTE_ASSUME(r_size - max_dist_inc < l_size)
        if (l_size <= max_dist_inc) {
            for (; i != max_dist_inc; ++i) {
                do_loop(p, d, i, get_zero, get_l_size, ignore);
                std::swap(p, d);
            }
            for (; i != r_size; ++i) {
                do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                std::swap(p, d);
            }
        } else {
            if (l_size > 2 * max_dist_inc) {
                for (; i != max_dist_inc; ++i) {
                    do_loop(p, d, i, get_zero, get_nl_max, ignore);
                    std::swap(p, d);
                }
                for (; i != l_size - max_dist_inc; ++i) {
                    do_loop(p, d, i, get_n0_min, get_nl_max, set_max);
                    std::swap(p, d);
                }
                for (; i != r_size; ++i) {
                    do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                    std::swap(p, d);
                }
            } else {
                for (; i != l_size - (max_dist + 1); ++i) {
                    do_loop(p, d, i, get_zero, get_nl_max, ignore);
                    std::swap(p, d);
                }
                for (; i != max_dist + 1; ++i) {
                    do_loop(p, d, i, get_zero, get_l_size, ignore);
                    std::swap(p, d);
                }
                for (; i != r_size; ++i) {
                    do_loop(p, d, i, get_n0_min, get_l_size, set_max);
                    std::swap(p, d);
                }
            }
        }
    }

    return p[l_size];
}

}  // namespace

/* Optimized Levenshtein distance computation algorithm that uses preallocated buffers and maximum
 * distance cutoff. Implementation is provided by @BUYT-1.
 */
std::size_t LevenshteinDistance(std::string const* l_ptr, std::string const* r_ptr, unsigned* p,
                                unsigned* d, std::size_t max_dist,
                                std::size_t fail_value) noexcept {
    std::size_t r_size = r_ptr->size();
    std::size_t l_size = l_ptr->size();
    if (l_size > r_size) {
        std::swap(l_ptr, r_ptr);
        std::swap(l_size, r_size);
    }
    if (r_size - l_size > max_dist) return fail_value;

    auto* l = &l_ptr->front();
    auto* r = &r_ptr->front();

    while (l_size != 0 && l[l_size - 1] == r[r_size - 1]) {
        --r_size;
        --l_size;
    }

    while (l_size != 0 && *l == *r) {
        --r_size;
        --l_size;
        ++l;
        ++r;
    }

    if (l_size == 0) return r_size;

    return LevenshteinDistanceMain(p, d, max_dist, l, l_size, r, r_size);
}

}  // namespace util
