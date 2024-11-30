#include "algorithms/md/hymd/preprocessing/column_matches/levenshtein.h"

#include <algorithm>
#include <cstddef>
#include <numeric>

#include "algorithms/md/hymd/lowest_bound.h"
#include "model/index.h"
#include "model/types/string_type.h"
#include "util/desbordante_assume.h"

namespace {
using namespace algos::hymd;

// TODO: very messy, clean up.
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
}  // namespace

namespace algos::hymd::preprocessing::column_matches {

std::size_t detail::LevenshteinComparerCreator::GetLargestStringSize(
        std::vector<model::String> const& elements) {
    DESBORDANTE_ASSUME(!elements.empty());
    return std::ranges::max(elements | std::views::transform(std::mem_fn(&model::String::size)));
}

preprocessing::Similarity detail::LevenshteinComparerCreator::Comparer::operator()(
        model::String const& l, model::String const& r) {
    std::size_t const max_dist = std::max(l.size(), r.size());
    Similarity similarity = 1.0;
    if (max_dist != 0) {
        std::size_t lim = max_dist * (1 - min_sim_);
        std::size_t dist = LevenshteinDistance(&l, &r, buf.get(), r_buf, lim, max_dist);
        similarity = (max_dist - dist) / static_cast<Similarity>(max_dist);
        if (similarity < min_sim_) similarity = kLowestBound;
    }
    return similarity;
}

Levenshtein::Levenshtein(ColumnIdentifier left_column_identifier,
                         ColumnIdentifier right_column_identifier,
                         model::md::DecisionBoundary min_sim,
                         ccv_id_pickers::SimilaritiesPicker picker,
                         detail::LevenshteinTransformer::TransformFunctionsOption funcs)
    : detail::LevenshteinBase(true, kName, std::move(left_column_identifier),
                              std::move(right_column_identifier), {std::move(funcs)},
                              {min_sim, std::move(picker)}) {}

Levenshtein::Levenshtein(ColumnIdentifier left_column_identifier,
                         ColumnIdentifier right_column_identifier,
                         model::md::DecisionBoundary min_sim, std::size_t size_limit,
                         detail::LevenshteinTransformer::TransformFunctionsOption funcs)
    : detail::LevenshteinBase(true, kName, std::move(left_column_identifier),
                              std::move(right_column_identifier), {std::move(funcs)},
                              {min_sim, ccv_id_pickers::IndexUniform<Similarity>(size_limit)}) {}

}  // namespace algos::hymd::preprocessing::column_matches
