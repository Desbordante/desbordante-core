#include "algorithms/ar_algorithm.h"

#include <algorithm>
#include <cassert>

#include <easylogging++.h>

#include "algorithms/options/names_and_descriptions.h"

namespace algos {

decltype(ARAlgorithm::InputFormatOpt) ARAlgorithm::InputFormatOpt{
        {config::names::kInputFormat, config::descriptions::kDInputFormat}};

decltype(ARAlgorithm::TidColumnIndexOpt) ARAlgorithm::TidColumnIndexOpt{
        {config::names::kTIdColumnIndex, config::descriptions::kDTIdColumnIndex}, 0};

decltype(ARAlgorithm::ItemColumnIndexOpt) ARAlgorithm::ItemColumnIndexOpt{
        {config::names::kItemColumnIndex, config::descriptions::kDItemColumnIndex}, 1};

decltype(ARAlgorithm::FirstColumnTidOpt) ARAlgorithm::FirstColumnTidOpt{
        {config::names::kFirstColumnTId, config::descriptions::kDFirstColumnTId}, false};

decltype(ARAlgorithm::MinSupportOpt) ARAlgorithm::MinSupportOpt{
        {config::names::kMinimumSupport, config::descriptions::kDMinimumSupport}, 0.0};

decltype(ARAlgorithm::MinConfidenceOpt) ARAlgorithm::MinConfidenceOpt{
        {config::names::kMinimumConfidence, config::descriptions::kDMinimumConfidence}, 0.0};

ARAlgorithm::ARAlgorithm(std::vector<std::string_view> phase_names)
        : Primitive(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(InputFormatOpt));
}

void ARAlgorithm::RegisterOptions() {
    auto sing_eq = [](InputFormat value) { return value == +InputFormat::singular; };
    auto tab_eq = [](InputFormat value) { return value == +InputFormat::tabular; };
    RegisterOption(InputFormatOpt.GetOption(&input_format_).SetConditionalOpts(
            GetOptAvailFunc(),
            {
                    {sing_eq, config::GetOptionNames(TidColumnIndexOpt, ItemColumnIndexOpt)},
                    {tab_eq, config::GetOptionNames(FirstColumnTidOpt)},
            }));
    RegisterOption(TidColumnIndexOpt.GetOption(&tid_column_index_));
    RegisterOption(ItemColumnIndexOpt.GetOption(&item_column_index_));
    RegisterOption(FirstColumnTidOpt.GetOption(&first_column_tid_));
    RegisterOption(MinSupportOpt.GetOption(&minsup_));
    RegisterOption(MinConfidenceOpt.GetOption(&minconf_));
}

void ARAlgorithm::ResetState() {
    ar_collection_.clear();
    ResetStateAr();
}

void ARAlgorithm::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(MinSupportOpt, MinConfidenceOpt));
}

void ARAlgorithm::FitInternal(model::IDatasetStream& data_stream) {
    switch (input_format_) {
        case InputFormat::singular:
            transactional_data_ = model::TransactionalData::CreateFromSingular(
                    data_stream, tid_column_index_, item_column_index_);
            break;
        case InputFormat::tabular:
            transactional_data_ =
                    model::TransactionalData::CreateFromTabular(data_stream, first_column_tid_);
            break;
        default:
            assert(0);
    }
    if (transactional_data_->GetNumTransactions() == 0) {
        throw std::runtime_error("Got an empty dataset: AR mining is meaningless.");
    }
}

unsigned long long ARAlgorithm::ExecuteInternal() {
    auto time = FindFrequent();
    time += GenerateAllRules();

    LOG(INFO) << "> Count of association rules: " << ar_collection_.size();
    return time;
}

void ARAlgorithm::UpdatePath(std::stack<RuleNode*>& path, std::list<RuleNode>& vertices) {
    for (auto iter = vertices.rbegin(); iter != vertices.rend(); ++iter) {
        RuleNode* node_ptr = &(*iter);
        path.push(node_ptr);
    }
}

void ARAlgorithm::GenerateRulesFrom(std::vector<unsigned> const& frequent_itemset, double support) {
    root_.children.clear();
    for (auto item_id : frequent_itemset) {
        std::vector<unsigned> rhs{item_id};
        std::vector<unsigned> lhs;
        std::set_difference(frequent_itemset.begin(), frequent_itemset.end(),
                            rhs.begin(), rhs.end(),
                            std::back_inserter(lhs));
        auto const lhs_support = GetSupport(lhs);
        auto const confidence = support / lhs_support;
        if (confidence >= minconf_) {
            auto const& new_ar = ar_collection_.emplace_back(std::move(lhs), std::move(rhs),
                                                             confidence);
            root_.children.emplace_back(new_ar);
        }
    }
    if (root_.children.empty()) {
        return;
    }

    unsigned level_number = 2;
    while (GenerateRuleLevel(frequent_itemset, support, level_number)) {
        ++level_number;
    }
}

bool ARAlgorithm::GenerateRuleLevel(std::vector<unsigned> const& frequent_itemset, double support,
                                    unsigned level_number) {
    bool generated_any = false;
    std::stack<RuleNode*> path;
    path.push(&root_);

    assert(level_number >= 2);
    while (!path.empty()) {
        // TODO(alexandrsmirn) попробовать как-то синхронизировать путь с генерацией
        auto node = path.top();
        path.pop();
        if (node->rule.right.size() == level_number - 2) {  // levelNumber is at least 2
            generated_any = MergeRules(frequent_itemset, support, node);
        } else {
            UpdatePath(path, node->children);
        }
    }

    return generated_any;
}

bool ARAlgorithm::MergeRules(std::vector<unsigned> const& frequent_itemset, double support,
                             RuleNode* node) {
    auto& children = node->children;
    bool is_rule_produced = false;

    auto const last_child_iter = std::prev(children.end());
    for (auto child_iter = children.begin(); child_iter != last_child_iter; ++child_iter) {
        for (auto child_right_sibling_iter = std::next(child_iter);
                  child_right_sibling_iter != children.end(); ++child_right_sibling_iter) {
            std::vector<unsigned> rhs = child_iter->rule.right;
            rhs.push_back(child_right_sibling_iter->rule.right.back());
            if (rhs.size() == frequent_itemset.size()) {
                // in this case the LHS is empty
                continue;
            }

            std::vector<unsigned> lhs;
            std::set_difference(frequent_itemset.begin(), frequent_itemset.end(),
                                rhs.begin(), rhs.end(),
                                std::back_inserter(lhs));

            auto const lhs_support = GetSupport(lhs);
            auto const confidence = support / lhs_support;
            if (confidence >= minconf_) {
                auto const& new_ar = ar_collection_.emplace_back(std::move(lhs), std::move(rhs),
                                                                 confidence);
                child_iter->children.emplace_back(new_ar);
                is_rule_produced = true;
            }
        }
    }
    return is_rule_produced;
}

std::list<model::ARStrings> ARAlgorithm::GetArStringsList() const {
    std::list<model::ARStrings> ar_strings;
    for (auto const& ar : ar_collection_) {
        ar_strings.emplace_back(ar, transactional_data_.get());
    }
    return ar_strings;
}

} // namespace algos
