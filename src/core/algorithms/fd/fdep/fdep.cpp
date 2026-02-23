#include "core/algorithms/fd/fdep/fdep.h"

#include <chrono>

#include "core/config/equal_nulls/option.h"
#include "core/config/max_lhs/option.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/model/table/column_layout_relation_data.h"
#include "core/model/types/bitset.h"

namespace algos {

FDep::FDep() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void FDep::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kMaxLhsOpt(&max_lhs_));
}

void FDep::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::kMaxLhsOpt.GetName()});
}

void FDep::LoadDataInternal() {
    std::size_t const attr_num = input_table_->GetNumberOfColumns();
    if (attr_num == 0) {
        throw std::runtime_error("Unable to work on an empty dataset.");
    }

    std::vector<std::string> column_names;
    column_names.reserve(attr_num);
    for (size_t i = 0; i != attr_num; ++i) {
        column_names.push_back(input_table_->GetColumnName(static_cast<int>(i)));
    }
    table_header_ = {input_table_->GetRelationName(), std::move(column_names)};

    std::vector<std::string> next_line;
    while (input_table_->HasNextRow()) {
        next_line = input_table_->GetNextRow();
        if (next_line.empty()) break;
        tuples_.emplace_back(std::vector<size_t>(attr_num));
        for (size_t i = 0; i != attr_num; ++i) {
            tuples_.back()[i] = std::hash<std::string>{}(next_line[i]);
        }
    }
}

void FDep::ResetState() {
    fd_storage_ = nullptr;
    // Consider creating them here instead.
    neg_cover_tree_.reset();
    pos_cover_tree_.reset();
}

unsigned long long FDep::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();

    MultiAttrRhsFdStorage::LhsLimBuilder storage_builder{max_lhs_};

    BuildNegativeCover();

    this->tuples_.shrink_to_fit();

    this->pos_cover_tree_ = std::make_unique<FDTreeElement>(table_header_.column_names.size());
    this->pos_cover_tree_->AddMostGeneralDependencies();

    model::Bitset<FDTreeElement::kMaxAttrNum> active_path;
    CalculatePositiveCover(*this->neg_cover_tree_, active_path);

    pos_cover_tree_->CreateAnswer(table_header_.column_names.size(), storage_builder, max_lhs_);

    fd_storage_ = storage_builder.Build(table_header_);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();
}

void FDep::BuildNegativeCover() {
    this->neg_cover_tree_ = std::make_unique<FDTreeElement>(table_header_.column_names.size());
    for (auto i = this->tuples_.begin(); i != this->tuples_.end(); ++i) {
        for (auto j = i + 1; j != this->tuples_.end(); ++j) AddViolatedFDs(*i, *j);
    }

    this->neg_cover_tree_->FilterSpecializations();
}

void FDep::AddViolatedFDs(std::vector<size_t> const& t1, std::vector<size_t> const& t2) {
    model::Bitset<FDTreeElement::kMaxAttrNum> equal_attr((2 << table_header_.column_names.size()) -
                                                         1);
    equal_attr.reset(0);
    model::Bitset<FDTreeElement::kMaxAttrNum> diff_attr;

    std::size_t const attr_num = table_header_.column_names.size();
    for (size_t attr = 0; attr < attr_num; ++attr) {
        diff_attr[attr + 1] = (t1[attr] != t2[attr]);
    }

    equal_attr &= (~diff_attr);
    for (size_t attr = diff_attr._Find_first(); attr != FDTreeElement::kMaxAttrNum;
         attr = diff_attr._Find_next(attr)) {
        this->neg_cover_tree_->AddFunctionalDependency(equal_attr, attr);
    }
}

void FDep::CalculatePositiveCover(FDTreeElement const& neg_cover_subtree,
                                  model::Bitset<FDTreeElement::kMaxAttrNum>& active_path) {
    std::size_t const attr_num = table_header_.column_names.size();
    for (size_t attr = 1; attr <= attr_num; ++attr) {
        if (neg_cover_subtree.CheckFd(attr - 1)) {
            this->SpecializePositiveCover(active_path, attr);
        }
    }

    for (size_t attr = 1; attr <= attr_num; ++attr) {
        if (neg_cover_subtree.GetChild(attr - 1)) {
            active_path.set(attr);
            this->CalculatePositiveCover(*neg_cover_subtree.GetChild(attr - 1), active_path);
            active_path.reset(attr);
        }
    }
}

void FDep::SpecializePositiveCover(model::Bitset<FDTreeElement::kMaxAttrNum> const& lhs,
                                   size_t const& a) {
    model::Bitset<FDTreeElement::kMaxAttrNum> spec_lhs;

    std::size_t const attr_num = table_header_.column_names.size();
    while (this->pos_cover_tree_->GetGeneralizationAndDelete(lhs, a, 0, spec_lhs)) {
        for (size_t attr = attr_num; attr > 0; --attr) {
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
