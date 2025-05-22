#pragma once

#include <cstddef>
#include <string>

#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"
#include "algorithms/mde/hymde/records/dictionary_compressed_records.h"
#include "model/index.h"

namespace algos::hymde::record_match_indexes::calculators {
class RecordView {
    using ClusterIter = PartitionIndex::Clusters::const_iterator;

    PartitionIndex::Clusters const& clusters_;
    records::DictionaryCompressed::Values const& values_;

public:
    class ValueIterator {
        ClusterIter cluster_iter_;
        records::DictionaryCompressed::Values const& values_;

        ValueIterator(ClusterIter cluster_iter, records::DictionaryCompressed::Values const& values)
            : cluster_iter_(cluster_iter), values_(values) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::string;
        using pointer = std::string const*;
        using reference = std::string const&;

        ValueIterator operator++() noexcept {
            ++cluster_iter_;
            return *this;
        }

        ValueIterator operator++(int) noexcept {
            ValueIterator old = *this;
            ++(*this);
            return old;
        }

        friend bool operator==(ValueIterator const& iter1, ClusterIter const& iter2) {
            return iter1.cluster_iter_ == iter2;
        }

        friend bool operator!=(ValueIterator const& iter1, ClusterIter const& iter2) {
            return !(iter1 == iter2);
        }

        reference operator*() const noexcept {
            return values_[*cluster_iter_];
        }

        friend RecordView;
    };

    RecordView(PartitionIndex::Clusters const& clusters,
               records::DictionaryCompressed::Values const& values)
        : clusters_(clusters), values_(values) {}

    std::string const& operator[](model::Index index) {
        return values_[clusters_[index]];
    }

    std::size_t Size() const noexcept {
        return clusters_.size();
    }

    ValueIterator begin() const noexcept {
        return {clusters_.begin(), values_};
    }

    ClusterIter end() const noexcept {
        return clusters_.end();
    }
};
}  // namespace algos::hymde::record_match_indexes::calculators
