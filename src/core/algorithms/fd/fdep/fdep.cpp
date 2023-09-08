#include "algorithms/fd/fdep/fdep.h"

#include <chrono>

#include "model/table/column_layout_relation_data.h"

//#ifndef PRINT_FDS
//#define PRINT_FDS
//#endif

namespace algos {

FDep::FDep() : FDAlgorithm({kDefaultPhaseName}) {}

void FDep::LoadDataInternal() {
    number_attributes_ = input_table_->GetNumberOfColumns();
    if (number_attributes_ == 0) {
        throw std::runtime_error("Unable to work on an empty dataset.");
    }
    column_names_.resize(number_attributes_);

    schema_ = std::make_unique<RelationalSchema>(input_table_->GetRelationName());

    for (size_t i = 0; i < number_attributes_; ++i) {
        column_names_[i] = input_table_->GetColumnName(static_cast<int>(i));
        schema_->AppendColumn(column_names_[i]);
    }

    std::vector<std::string> next_line;
    while (input_table_->HasNextRow()) {
        next_line = input_table_->GetNextRow();
        if (next_line.empty()) break;
        tuples_.emplace_back(std::vector<size_t>(number_attributes_));
        for (size_t i = 0; i < number_attributes_; ++i) {
            tuples_.back()[i] = std::hash<std::string>{}(next_line[i]);
        }
    }
}

void FDep::ResetStateFd() {
    // Consider creating them here instead.
    neg_cover_tree_.reset();
    pos_cover_tree_.reset();
}

unsigned long long FDep::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    BuildNegativeCover();

    this->tuples_.shrink_to_fit();

    this->pos_cover_tree_ = std::make_unique<FDTreeElement>(this->number_attributes_);
    this->pos_cover_tree_->AddMostGeneralDependencies();

    std::bitset<FDTreeElement::kMaxAttrNum> active_path;
    CalculatePositiveCover(*this->neg_cover_tree_, active_path);

    pos_cover_tree_->FillFdCollection(*this->schema_, FdList());

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);

#ifdef PRINT_FDS
    pos_cover_tree_->printDep("recent_call_result.txt", this->column_names_);
#endif

    return elapsed_milliseconds.count();
}

void FDep::BuildNegativeCover() {
    this->neg_cover_tree_ = std::make_unique<FDTreeElement>(this->number_attributes_);
    for (auto i = this->tuples_.begin(); i != this->tuples_.end(); ++i) {
        for (auto j = i + 1; j != this->tuples_.end(); ++j)
            AddViolatedFDs(*i, *j);
    }

    this->neg_cover_tree_->FilterSpecializations();
}

void FDep::AddViolatedFDs(std::vector<size_t> const& t1, std::vector<size_t> const& t2) {
    std::bitset<FDTreeElement::kMaxAttrNum> equal_attr((2 << this->number_attributes_) - 1);
    equal_attr.reset(0);
    std::bitset<FDTreeElement::kMaxAttrNum> diff_attr;

    for (size_t attr = 0; attr < this->number_attributes_; ++attr) {
        diff_attr[attr + 1] = (t1[attr] != t2[attr]);
    }

    equal_attr &= (~diff_attr);
    for (size_t attr = diff_attr._Find_first(); attr != FDTreeElement::kMaxAttrNum;
         attr = diff_attr._Find_next(attr)) {
        this->neg_cover_tree_->AddFunctionalDependency(equal_attr, attr);
    }
}

void FDep::CalculatePositiveCover(FDTreeElement const& neg_cover_subtree,
                                  std::bitset<FDTreeElement::kMaxAttrNum>& active_path) {
    for (size_t attr = 1; attr <= this->number_attributes_; ++attr) {
        if (neg_cover_subtree.CheckFd(attr - 1)) {
            this->SpecializePositiveCover(active_path, attr);
        }
    }

    for (size_t attr = 1; attr <= this->number_attributes_; ++attr) {
        if (neg_cover_subtree.GetChild(attr - 1)) {
            active_path.set(attr);
            this->CalculatePositiveCover(*neg_cover_subtree.GetChild(attr - 1), active_path);
            active_path.reset(attr);
        }
    }
}

void FDep::SpecializePositiveCover(std::bitset<FDTreeElement::kMaxAttrNum> const& lhs,
                                   size_t const& a) {
    std::bitset<FDTreeElement::kMaxAttrNum> spec_lhs;

    while (this->pos_cover_tree_->GetGeneralizationAndDelete(lhs, a, 0, spec_lhs)) {
        for (size_t attr = this->number_attributes_; attr > 0; --attr) {
            if (!lhs.test(attr) && (attr != a)) {
                spec_lhs.set(attr);
                if (!this->pos_cover_tree_->ContainsGeneralization(spec_lhs, a, 0)) {
                    this->pos_cover_tree_->AddFunctionalDependency(spec_lhs, a);
                }
                spec_lhs.reset(attr);
            }
        }

        spec_lhs.reset();
    }
}

}  // namespace algos
