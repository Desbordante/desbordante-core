#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <sstream>
#include <cstddef>
#include <numeric>

#include "algorithms/md/hymd/preprocessing/similarity_measure/immediate_similarity_measure.h"
#include "config/exceptions.h"

unsigned LevenshteinDistance(std::byte* l_ptr, std::byte* r_ptr) noexcept {
    std::string_view l(reinterpret_cast<char*>(l_ptr));
    std::string_view r(reinterpret_cast<char*>(r_ptr));

    std::size_t r_size = r.size();
    std::size_t l_size = l.size();

    unsigned v0[r_size + 1];
    unsigned v1[r_size + 1];

    if (r_size > l_size) {
        std::swap(l, r);
        std::swap(l_size, r_size);
    }

    std::iota(v0, v0 + r_size + 1, 0);

    auto compute_arrays = [&](unsigned* v0, unsigned* v1, unsigned i) {
        *v1 = i + 1;
        char const li = l[i];

        for (unsigned j = 0; j != r_size;) {
            unsigned const insert_cost = v1[j] + 1;
            unsigned const substition_cost = v0[j] + (li != r[j]);
            ++j;
            unsigned const deletion_cost = v0[j] + 1;

            v1[j] = std::min({deletion_cost, insert_cost, substition_cost});
        }
    };

    auto loop_to_l_size = [&l_size, &v0, &v1, &compute_arrays]() {
        for (unsigned i = 0; i != l_size; ++i) {
            compute_arrays(v0, v1, i);
            ++i;
            compute_arrays(v1, v0, i);
        }
    };

    if (l_size & 1) {
        --l_size;
        loop_to_l_size();
        compute_arrays(v0, v1, l_size);
        return v1[r_size];
    } else {
        loop_to_l_size();
        return v0[r_size];
    }
}

double JacaardMetric(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    std::unordered_set<char> set1(str1.begin(), str1.end());
    std::unordered_set<char> set2(str2.begin(), str2.end());

    std::unordered_set<char> intersection;
    for (char ch : set1) {
        if (set2.find(ch) != set2.end()) {
            intersection.insert(ch);
        }
    }

    double intersection_size = static_cast<double>(intersection.size());
    double union_size = static_cast<double>(set1.size() + set2.size() - intersection_size);

    return union_size == 0 ? 0.0 : 1 - intersection_size / union_size;
}

double longest_common_subsequence(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    size_t m = str1.size();
    size_t n = str2.size();

    std::vector<std::vector<size_t>> lcsTable(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (str1[i - 1] == str2[j - 1]) {
                lcsTable[i][j] = lcsTable[i - 1][j - 1] + 1;
            } else {
                lcsTable[i][j] = std::max(lcsTable[i - 1][j], lcsTable[i][j - 1]);
            }
        }
    }

    return lcsTable[m][n];
}

std::byte* stringToBytes(const std::string& str) {
    return reinterpret_cast<std::byte*>(const_cast<char*>(str.data()));
}

double MongeElkan(const std::byte* word1, const std::byte* word2) {
    std::string str1(reinterpret_cast<const char*>(word1));
    std::string str2(reinterpret_cast<const char*>(word2));

    double cummax = 0.0;
    std::istringstream iss1(str1), iss2(str2);
    std::string token1, token2;

    while (std::getline(iss1, token1, ' ')) {
        double maxscore = 0.0;
        iss2.clear();
        iss2.seekg(0);

        while (std::getline(iss2, token2, ' ')) {
            maxscore = std::max(maxscore, JacaardMetric(reinterpret_cast<const std::byte*>(token1.c_str()), reinterpret_cast<const std::byte*>(token2.c_str())));
        }
        cummax += maxscore;
    }
    return cummax / std::count(str1.begin(), str1.end(), ' ') + 1;
}

namespace algos::hymd::preprocessing::similarity_measure {
using SimInfo = std::map<Similarity, indexes::RecSet>;

indexes::ColumnMatchSimilarityInfo ImmediateSimilarityMeasure::MakeIndexes(
        std::shared_ptr<DataInfo const> data_info_left,
        std::shared_ptr<DataInfo const> data_info_right,
        std::vector<indexes::PliCluster> const* clusters_right, model::md::DecisionBoundary min_sim, bool is_null_equal_null) const {
    std::vector<model::md::DecisionBoundary> decision_bounds;
    indexes::SimilarityMatrix similarity_matrix;
    indexes::SimilarityIndex similarity_index;
    auto const& data_left_size = data_info_left->GetElementNumber();
    auto const& data_right_size = data_info_right->GetElementNumber();
    Similarity lowest = 1.0;
    for (ValueIdentifier value_id_left = 0; value_id_left < data_left_size; ++value_id_left) {
        auto const& string_left =
                    model::Type::GetValue<std::string>(data_info_left->GetAt(value_id_left));
            std::size_t const left_size = string_left.size();
        std::vector<std::pair<Similarity, RecordIdentifier>> sim_rec_id_vec;
        auto get_similarity = [&string_left, left_size, &data_info_right, this](ValueIdentifier value_id_right) {

                auto const& string_right =
                        model::Type::GetValue<std::string>(data_info_right->GetAt(value_id_right));
                std::size_t const max_dist = std::max(left_size, string_right.size());
                Similarity value = compute_similarity_(stringToBytes(string_left) ,data_info_right->GetAt(value_id_right));
                if (string_left == string_right){
                    return 1.0;
                }
                if (value < 0 || value >= 1){
                    return static_cast<Similarity>(max_dist - value) / static_cast<Similarity>(max_dist);
                }
                return value;
            };
        for (ValueIdentifier value_id_right = 0; value_id_right < data_right_size;
             ++value_id_right) {
            Similarity similarity = get_similarity(value_id_right);
            if (similarity < min_sim) {
                // Metanome keeps the actual value for some reason.
                lowest = 0.0 /*similarity???*/;
                continue;
            }
            if (lowest > similarity) lowest = similarity;
            decision_bounds.push_back(similarity);
            similarity_matrix[value_id_left][value_id_right] = similarity;
            for (RecordIdentifier record_id : clusters_right->operator[](value_id_right)) {
                sim_rec_id_vec.emplace_back(similarity, record_id);
            }
        }
        if (sim_rec_id_vec.empty()) continue;
        std::sort(sim_rec_id_vec.begin(), sim_rec_id_vec.end(), std::greater<>{});
        std::vector<RecordIdentifier> records;
        records.reserve(sim_rec_id_vec.size());
        for (auto [_, rec] : sim_rec_id_vec) {
            records.push_back(rec);
        }
        SimInfo sim_info;
        Similarity previous_similarity = sim_rec_id_vec.begin()->first;
        auto const it_begin = records.begin();
        for (model::Index j = 0; j < sim_rec_id_vec.size(); ++j) {
            Similarity const similarity = sim_rec_id_vec[j].first;
            if (similarity == previous_similarity) continue;
            auto const it_end = it_begin + static_cast<long>(j);
            // TODO: use std::inplace_merge
            std::sort(it_begin, it_end);
            sim_info[previous_similarity] = {it_begin, it_end};
            previous_similarity = similarity;
        }
        std::sort(records.begin(), records.end());
        sim_info[previous_similarity] = {records.begin(), records.end()};
        similarity_index[value_id_left] = std::move(sim_info);
    }
    std::sort(decision_bounds.begin(), decision_bounds.end());
    decision_bounds.erase(std::unique(decision_bounds.begin(), decision_bounds.end()),
                          decision_bounds.end());
    return {std::move(decision_bounds), lowest, std::move(similarity_matrix),
            std::move(similarity_index)};
}

}  // namespace algos::hymd::preprocessing::similarity_measure
