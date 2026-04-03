#include "core/algorithms/fd/fdep/fd_tree_element.h"

#include "core/model/types/bitset.h"

namespace {
template <typename Bitset>
static boost::dynamic_bitset<> BitsetToDynamicBitset(Bitset const& b, std::size_t const size) {
    boost::dynamic_bitset<> dyn_bitset(size);
    for (std::size_t i = 0; i < size; ++i) {
        dyn_bitset.set(i, b[i]);
    }
    return dyn_bitset;
}
}  // namespace

FDTreeElement::FDTreeElement(size_t max_attribute_number)
    : max_attribute_number_(max_attribute_number) {
    children_.resize(max_attribute_number);
}

bool FDTreeElement::CheckFd(size_t index) const {
    return this->is_fd_[index];
}

FDTreeElement* FDTreeElement::GetChild(size_t index) const {
    return this->children_[index].get();
}

void FDTreeElement::AddRhsAttribute(size_t index) {
    this->rhs_attributes_.set(index);
}

model::Bitset<FDTreeElement::kMaxAttrNum> const& FDTreeElement::GetRhsAttributes() const {
    return this->rhs_attributes_;
}

void FDTreeElement::MarkAsLast(size_t index) {
    this->is_fd_.set(index);
}

bool FDTreeElement::IsFinalNode(size_t attr_num) const {
    if (!this->rhs_attributes_[attr_num]) {
        return false;
    }
    for (size_t attr = 0; attr < this->max_attribute_number_; ++attr) {
        if (children_[attr] && children_[attr]->GetRhsAttributes()[attr_num]) {
            return false;
        }
    }
    return true;
}

bool FDTreeElement::ContainsGeneralization(model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs,
                                           size_t attr_num, size_t current_attr) const {
    if (this->is_fd_[attr_num - 1]) {
        return true;
    }

    size_t next_set_attr = lhs._Find_next(current_attr);
    if (next_set_attr == kMaxAttrNum) {
        return false;
    }
    bool found = false;
    if (this->children_[next_set_attr - 1] &&
        this->children_[next_set_attr - 1]->GetRhsAttributes()[attr_num]) {
        found = this->children_[next_set_attr - 1]->ContainsGeneralization(lhs, attr_num,
                                                                           next_set_attr);
    }

    if (found) {
        return true;
    }
    return this->ContainsGeneralization(lhs, attr_num, next_set_attr);
}

bool FDTreeElement::GetGeneralizationAndDelete(
        model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs, size_t attr_num, size_t current_attr,
        model::Bitset<FDTreeElement::kMaxAttrNum>& spec_lhs) {
    if (this->is_fd_[attr_num - 1]) {
        this->is_fd_.reset(attr_num - 1);
        this->rhs_attributes_.reset(attr_num);
        return true;
    }

    size_t next_set_attr = lhs._Find_next(current_attr);
    if (next_set_attr == kMaxAttrNum) {
        return false;
    }

    bool found = false;
    if (this->children_[next_set_attr - 1] &&
        this->children_[next_set_attr - 1]->GetRhsAttributes()[attr_num]) {
        found = this->children_[next_set_attr - 1]->GetGeneralizationAndDelete(
                lhs, attr_num, next_set_attr, spec_lhs);
        if (found) {
            if (this->IsFinalNode(attr_num)) {
                this->rhs_attributes_.reset(attr_num);
            }

            spec_lhs.set(next_set_attr);
        }
    }
    if (!found) {
        found = this->GetGeneralizationAndDelete(lhs, attr_num, next_set_attr, spec_lhs);
    }
    return found;
}

bool FDTreeElement::GetSpecialization(
        model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs, size_t attr_num, size_t current_attr,
        model::Bitset<FDTreeElement::kMaxAttrNum>& spec_lhs_out) const {
    if (!this->rhs_attributes_[attr_num]) {
        return false;
    }

    bool found = false;
    size_t attr = (current_attr > 1 ? current_attr : 1);
    size_t next_set_attr = lhs._Find_next(current_attr);

    if (next_set_attr == kMaxAttrNum) {
        while (!found && attr <= this->max_attribute_number_) {
            if (this->children_[attr - 1] &&
                this->children_[attr - 1]->GetRhsAttributes()[attr_num]) {
                found = this->children_[attr - 1]->GetSpecialization(lhs, attr_num, current_attr,
                                                                     spec_lhs_out);
            }
            ++attr;
        }
        if (found) {
            spec_lhs_out.set(attr - 1);
        }
        return true;
    }

    while (!found && attr < next_set_attr) {
        if (this->children_[attr - 1] && this->children_[attr - 1]->GetRhsAttributes()[attr_num]) {
            found = this->children_[attr - 1]->GetSpecialization(lhs, attr_num, current_attr,
                                                                 spec_lhs_out);
        }
        ++attr;
    }
    if (!found && this->children_[next_set_attr - 1] &&
        this->children_[next_set_attr - 1]->GetRhsAttributes()[attr_num]) {
        found = this->children_[next_set_attr - 1]->GetSpecialization(lhs, attr_num, next_set_attr,
                                                                      spec_lhs_out);
    }

    spec_lhs_out.set(attr - 1, found);

    return found;
}

void FDTreeElement::AddMostGeneralDependencies() {
    for (size_t i = 1; i <= this->max_attribute_number_; ++i) {
        this->rhs_attributes_.set(i);
    }

    for (size_t i = 0; i < this->max_attribute_number_; ++i) {
        this->is_fd_[i] = true;
    }
}

void FDTreeElement::AddFunctionalDependency(model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs,
                                            size_t attr_num) {
    FDTreeElement* current_node = this;
    this->AddRhsAttribute(attr_num);

    for (size_t i = lhs._Find_first(); i != kMaxAttrNum; i = lhs._Find_next(i)) {
        if (current_node->children_[i - 1] == nullptr) {
            current_node->children_[i - 1] =
                    std::make_unique<FDTreeElement>(this->max_attribute_number_);
        }

        current_node = current_node->GetChild(i - 1);
        current_node->AddRhsAttribute(attr_num);
    }

    current_node->MarkAsLast(attr_num - 1);
}

void FDTreeElement::FilterSpecializations() {
    model::Bitset<kMaxAttrNum> active_path;
    auto filtered_tree = std::make_unique<FDTreeElement>(this->max_attribute_number_);

    this->FilterSpecializationsHelper(*filtered_tree, active_path);

    this->children_ = std::move(filtered_tree->children_);
    this->is_fd_ = filtered_tree->is_fd_;
}

void FDTreeElement::FilterSpecializationsHelper(
        FDTreeElement& filtered_tree, model::Bitset<FDTreeElement::kMaxAttrNum>& active_path) {
    for (size_t attr = 1; attr <= this->max_attribute_number_; ++attr) {
        if (this->children_[attr - 1]) {
            active_path.set(attr);
            this->children_[attr - 1]->FilterSpecializationsHelper(filtered_tree, active_path);
            active_path.reset(attr);
        }
    }

    for (size_t attr = 1; attr <= this->max_attribute_number_; ++attr) {
        model::Bitset<kMaxAttrNum> spec_lhs_out;
        if (this->is_fd_[attr - 1] &&
            !filtered_tree.GetSpecialization(active_path, attr, 0, spec_lhs_out)) {
            filtered_tree.AddFunctionalDependency(active_path, attr);
        }
    }
}

void FDTreeElement::CreateAnswer(std::size_t attr_num,
                                 algos::MultiAttrRhsFdStorage::LhsLimBuilder& builder,
                                 unsigned int max_lhs) const {
    boost::dynamic_bitset<> lhs(attr_num);
    this->TransformTreeFdCollection(lhs, builder, max_lhs);
}

void FDTreeElement::TransformTreeFdCollection(boost::dynamic_bitset<>& lhs,
                                              algos::MultiAttrRhsFdStorage::LhsLimBuilder& builder,
                                              unsigned int max_lhs) const {
    if (lhs.count() > max_lhs) return;

    if (is_fd_.any()) builder.AddFd({lhs, BitsetToDynamicBitset(is_fd_, lhs.size())});

    for (size_t attr = 0; attr != lhs.size(); ++attr) {
        if (this->children_[attr]) {
            lhs.set(attr);
            this->children_[attr]->TransformTreeFdCollection(lhs, builder, max_lhs);
            lhs.reset(attr);
        }
    }
}
