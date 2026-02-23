#pragma once

#include <list>
#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fd.h"
#include "core/algorithms/fd/multi_attr_rhs_fd_storage.h"
#include "core/model/table/relational_schema.h"
#include "core/model/types/bitset.h"

class FDTreeElement {
public:
    // The maximum number of columns in the dataset. Using in std::bitset template.
    static constexpr int kMaxAttrNum = 256;

    explicit FDTreeElement(size_t max_attribute_number);

    FDTreeElement(FDTreeElement const&) = delete;
    FDTreeElement& operator=(FDTreeElement const&) = delete;

    void AddMostGeneralDependencies();

    // Using in cover-trees as post filtration of functional dependencies with redundant left-hand
    // side.
    void FilterSpecializations();

    [[nodiscard]] bool CheckFd(size_t index) const;

    [[nodiscard]] FDTreeElement* GetChild(size_t index) const;

    void AddFunctionalDependency(model::Bitset<kMaxAttrNum> const& lhs, size_t attr_num);

    // Searching for generalization of functional dependency in cover-trees.
    bool GetGeneralizationAndDelete(model::Bitset<kMaxAttrNum> const& lhs, size_t attr_num,
                                    size_t current_attr, model::Bitset<kMaxAttrNum>& spec_lhs);

    [[nodiscard]] bool ContainsGeneralization(model::Bitset<kMaxAttrNum> const& lhs,
                                              size_t attr_num, size_t current_attr) const;

    void CreateAnswer(std::size_t attr_num, algos::MultiAttrRhsFdStorage::LhsLimBuilder& builder,
                      unsigned int max_lhs = std::numeric_limits<unsigned int>::max()) const;

private:
    std::vector<std::unique_ptr<FDTreeElement>> children_;
    // TODO: why is this not dynamic_bitset?
    model::Bitset<kMaxAttrNum> rhs_attributes_;
    size_t max_attribute_number_;
    model::Bitset<kMaxAttrNum> is_fd_;

    void AddRhsAttribute(size_t index);

    [[nodiscard]] model::Bitset<kMaxAttrNum> const& GetRhsAttributes() const;

    void MarkAsLast(size_t index);

    // Checking whether node is a leaf or not.
    [[nodiscard]] bool IsFinalNode(size_t attr_num) const;

    // Searching for specialization of functional dependency in cover-trees.
    bool GetSpecialization(model::Bitset<kMaxAttrNum> const& lhs, size_t attr_num,
                           size_t current_attr, model::Bitset<kMaxAttrNum>& spec_lhs_out) const;

    void FilterSpecializationsHelper(FDTreeElement& filtered_tree,
                                     model::Bitset<kMaxAttrNum>& active_path);

    void TransformTreeFdCollection(
            boost::dynamic_bitset<>& lhs, algos::MultiAttrRhsFdStorage::LhsLimBuilder& builder,
            unsigned int max_lhs = std::numeric_limits<unsigned int>::max()) const;
};
