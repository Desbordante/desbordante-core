#include "core/algorithms/ind/faida/inclusion_testing/combined_inclusion_tester.h"

#ifdef __AVX2__
#include <immintrin.h>
#endif
#include "core/algorithms/ind/faida/hashing/hashing.h"
#include "core/util/parallel_for.h"

namespace algos::faida {

IInclusionTester::ActiveColumns CombinedInclusionTester::SetCCs(
        std::vector<std::shared_ptr<SimpleCC>>& combinations) {
    hlls_by_table_.clear();
    ActiveColumns active_columns_set;
    int index = 0;
    for (std::shared_ptr<SimpleCC> const& cc : combinations) {
        for (ColumnIndex col_idx : cc->GetColumnIndices()) {
            active_columns_set[cc->GetTableIndex()].insert(col_idx);
        }

        emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData>& hlls_by_cc =
                hlls_by_table_[cc->GetTableIndex()];
        if (hlls_by_cc.find(cc) == hlls_by_cc.end()) {
            cc->SetIndex(index++);
            hlls_by_cc[cc] = CreateApproxDataStructure();
        }
    }
    max_id_ = index;
    return active_columns_set;
}

void CombinedInclusionTester::Initialize(
        std::vector<IInclusionTester::HashedTableSample> const& samples_for_tables) {
    std::vector<size_t> samples;
    for (TableIndex table_idx = 0; table_idx < samples_for_tables.size(); table_idx++) {
        auto hll_by_cc_iter = hlls_by_table_.find(table_idx);

        if (hll_by_cc_iter != hlls_by_table_.end()) {
            emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData> const& hll_by_cc =
                    hll_by_cc_iter->second;
            HashedTableSample const& table_sample = samples_for_tables[table_idx];

            for (std::vector<size_t> const& sample_row : table_sample) {
                for (auto const& [cc, hll_data] : hll_by_cc) {
                    size_t combined_hash = 0;
                    bool is_any_null = false;

                    for (ColumnIndex col_idx : cc->GetColumnIndices()) {
                        size_t value_hash = sample_row[col_idx];
                        if (value_hash == null_hash_) {
                            is_any_null = true;
                            break;
                        }
                        combined_hash = std::rotl(combined_hash, 1) ^ value_hash;
                    }

                    if (!is_any_null) {
                        samples.push_back(combined_hash);
                    }
                }
            }
        }
    }
    sampled_inverted_index_.Init(samples, max_id_);
}

void CombinedInclusionTester::InsertRows(IRowIterator::Block const& hashed_cols,
                                         size_t block_size) {
    emhash8::HashMap<std::shared_ptr<SimpleCC>, HLLData>& hll_by_cc =
            hlls_by_table_[curr_table_idx_];
    unsigned int const chunk_size = block_size;

    util::ParallelForeach(
            hll_by_cc.begin(), hll_by_cc.end(), num_threads_,
            [this, &hashed_cols, chunk_size](auto& elem) {
                std::shared_ptr<SimpleCC>& cc = elem.first;
                HLLData& hll_data = elem.second;
                IRowIterator::AlignedVector combined_hashes(chunk_size, 0);
                std::vector<unsigned char> nul_combs(chunk_size, 0);

#ifdef __AVX2__
                __m256i const nullhashes_vect = _mm256_set1_epi64x(null_hash_);
                int constexpr vect_reg_size = 4;
#endif

                for (ColumnIndex col_idx : cc->GetColumnIndices()) {
                    IRowIterator::AlignedVector const& col_hashes_chunk =
                            hashed_cols[col_idx].value();
#ifdef __AVX2__
                    for (unsigned int row_offset = 0;
                         row_offset < chunk_size - chunk_size % vect_reg_size;
                         row_offset += vect_reg_size) {
                        __m256i const hashes_vect =
                                _mm256_load_si256((__m256i*)(&col_hashes_chunk[row_offset]));
                        __m256i const comb_hashes_vect =
                                _mm256_load_si256((__m256i*)(&combined_hashes[row_offset]));

                        __m256i const cmp_res = _mm256_cmpeq_epi64(hashes_vect, nullhashes_vect);
                        unsigned int const cmp_res_mask = _mm256_movemask_epi8(cmp_res);
                        (unsigned int&)*(&nul_combs[row_offset]) |= cmp_res_mask;

                        // Emulate rotl vector instruction
                        int constexpr num_shift_left = 1;
                        int constexpr num_shift_right = 63;
                        __m256i const sh_left_vect =
                                _mm256_slli_epi64(comb_hashes_vect, num_shift_left);
                        __m256i const sh_right_vect =
                                _mm256_srli_epi64(comb_hashes_vect, num_shift_right);
                        __m256i const rotated_hashes_vect =
                                _mm256_or_si256(sh_left_vect, sh_right_vect);

                        __m256i const xored_hashes_vect =
                                _mm256_xor_si256(rotated_hashes_vect, hashes_vect);
                        _mm256_store_si256((__m256i*)(&combined_hashes[row_offset]),
                                           xored_hashes_vect);
                    }

                    for (unsigned int row_offset = chunk_size - chunk_size % vect_reg_size;
                         row_offset < chunk_size; row_offset++) {
                        size_t const hash = col_hashes_chunk[row_offset];
                        size_t const comb_hash = combined_hashes[row_offset];

                        unsigned char const cmp_res = (hash == null_hash_ ? 0xFF : 0);
                        nul_combs[row_offset] |= cmp_res;

                        size_t const combined_hash = std::rotl(comb_hash, 1) ^ hash;
                        combined_hashes[row_offset] = combined_hash;
                    }
#else
                    for (unsigned int row_offset = 0; row_offset < chunk_size; row_offset++) {
                        size_t const hash = col_hashes_chunk[row_offset];
                        size_t const comb_hash = combined_hashes[row_offset];

                        unsigned char const cmp_res = (hash == null_hash_ ? 0xFF : 0);
                        nul_combs[row_offset] |= cmp_res;

                        size_t const combined_hash = std::rotl(comb_hash, 1) ^ hash;
                        combined_hashes[row_offset] = combined_hash;
                    }
#endif
                }

                for (unsigned int i = 0; i < chunk_size; i++) {
                    size_t const combined_hash = combined_hashes[i];
                    bool const has_null = nul_combs[i];
                    if (!has_null) {
                        if (!sampled_inverted_index_.Update(*cc, combined_hash)) {
                            /* Row is not found in the inverted index (cc is not covered)
                             * so we insert cc into hll if row does not contain null value
                             * and is not covered by inv_index
                             */
                            InsertRowIntoHLL(combined_hash, hll_data);
                        }
                    }
                }
            });
}

void CombinedInclusionTester::StartInsertRow(TableIndex table_idx) {
    curr_table_idx_ = table_idx;
}

bool CombinedInclusionTester::IsIncludedIn(std::shared_ptr<SimpleCC> const& dep,
                                           std::shared_ptr<SimpleCC> const& ref) {
    bool is_dep_covered = sampled_inverted_index_.IsCovered(dep);
    bool is_ref_covered = sampled_inverted_index_.IsCovered(ref);

    if (is_dep_covered) {
        num_certain_checks_++;
        return sampled_inverted_index_.IsIncludedIn(dep, ref);
    } else if (is_ref_covered) {
        num_certain_checks_++;
        return false;
    } else if (!sampled_inverted_index_.IsIncludedIn(dep, ref)) {
        num_certain_checks_++;
        return false;
    }

    num_uncertain_checks_++;
    return TestWithHLLs(dep, ref);
}

}  // namespace algos::faida
