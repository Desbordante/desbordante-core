#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "algorithms/near/types.h"

namespace kingfisher {

// Each node in the tree has a unique address that is an ascending sequence of OFeatureIndex-es.
class NodeAdress {
private:
    // TODO: replace with std::dequeue
    std::vector<OFeatureIndex> feat_i_vec_;

public:
    NodeAdress(std::vector<OFeatureIndex> vec);

    NodeAdress(OFeatureIndex ordered_feat_index) {
        feat_i_vec_ = {ordered_feat_index};
    }

    std::vector<OFeatureIndex> Get() const;
    std::vector<OFeatureIndex> GetExcept(size_t at) const;
    std::vector<OFeatureIndex> GetExceptFeat(OFeatureIndex feat) const;
    std::vector<NodeAdress> GetChildren(size_t feat_count) const;
    std::vector<OFeatureIndex> GetParent() const;
    std::vector<NodeAdress> GetParents() const;
    OFeatureIndex PopFront();
    OFeatureIndex Front() const;
    OFeatureIndex Back() const;
    bool Contains(OFeatureIndex feature) const;
    bool Increment(size_t feature_number);

    void EmplaceBack(OFeatureIndex i) {
        assert(i > Back());
        feat_i_vec_.emplace_back(i);
    }

    size_t Size() const;
    bool Empty() const;
    std::vector<FeatureIndex> ToFeatures(std::vector<FeatureIndex> const& order) const;
    std::string ToString() const;
};

}  // namespace kingfisher
