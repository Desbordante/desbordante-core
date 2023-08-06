#include <iostream>
#include <set>
#include <map>

#include "fastod.h"
#include "predicates/OperatorType.h"
#include "predicates/SingleAttributePredicate.h"

using namespace algos::fastod;

Fastod::Fastod(const DataFrame& data, long time_limit, double error_rate_threshold) noexcept :
    //Algorithm({"Mining ODs"}),
    data_(data),
    time_limit_(time_limit),
    error_rate_threshold_(error_rate_threshold) {}

Fastod::Fastod(const DataFrame& data, long time_limit) noexcept :
    //Algorithm({"Mining ODs"}),
    data_(data),
    time_limit_(time_limit) {}

bool Fastod::IsTimeUp() const noexcept {
    return timer_.GetElapsedSeconds() >= time_limit_;
}

void Fastod::CCPut(const AttributeSet& key, const AttributeSet& attribute_set) noexcept {
    cc_[key] = attribute_set;
}

void Fastod::CCPut(const AttributeSet& key, int attribute) noexcept {
    if (cc_.find(key) == cc_.end()) {
        cc_[key] = {};
    }

    cc_[key] = cc_[key].AddAttribute(attribute);
}

const AttributeSet& Fastod::CCGet(const AttributeSet& key) noexcept {
    if (cc_.find(key) == cc_.end()) {
        cc_[key] = {};
    }

    return cc_[key];
}

void Fastod::CSPut(const AttributeSet& key, const AttributePair& value) noexcept {
    if (cs_.find(key) == cs_.end()) {
        cs_[key] = {};
    }

    cs_[key].insert(value);
}

std::unordered_set<AttributePair>& Fastod::CSGet(const AttributeSet& key) noexcept {
    if (cs_.find(key) == cs_.end()) {
        cs_[key] = {};
    }

    return cs_[key];
}

void Fastod::PrintStatistics() const noexcept {
    std::string last_od = result_.size() > 0
        ? result_[result_.size() - 1].ToString()
        : std::string("");
    
    std::cout << "Current time " << timer_.GetElapsedSeconds() << " sec, "
              << "found od " << fd_count_ + ocd_count_ << " ones, where "
              << "fd " << fd_count_ << " ones, "
              << "ocd " << ocd_count_ << " ones, "
              << "the last od is " << last_od << '\n';
}

bool Fastod::IsComplete() const noexcept {
    return is_complete_;
}

void Fastod::Initialize() noexcept {
    timer_.Start();

    AttributeSet empty_set;
    context_in_each_level_.push_back(std::set<AttributeSet>());
    context_in_each_level_[0].insert(empty_set);

    for (int i = 0; i < data_.GetColumnCount(); i++) {
        schema_ = schema_.AddAttribute(i);
        CCPut(empty_set, i);
    }

    level_ = 1;

    std::set<AttributeSet> level_1_candidates;
    for (int i = 0; i < data_.GetColumnCount(); i++) {
        auto single_attribute = empty_set.AddAttribute(i);
        level_1_candidates.insert(single_attribute);
    }

    context_in_each_level_.push_back(level_1_candidates);
}

// void Fastod::PrintState() const noexcept {
//     std::cout << "-----------------------\n";
//     std::cout << "CONTEXT IN EACH LEVEL:\n";
//     for (auto context: context_in_each_level_) {
//         for (auto value: context) {
//             std::cout << value.ToString() << " ";
//         }
//         std::cout << "\n";
//     }
//
//     std::cout << "-----------------------\n";
//     std::cout << "CC:\n";
//     for (auto [key, value]: cc_) {
//         std::cout << key.ToString() << ": " << value.ToString() << "\n";
//     }
//
//     std::cout << "-----------------------\n";
//     std::cout << "CS:\n";
//     for (auto [key, values]: cs_) {
//         std::cout << key.ToString() << ": ";
//         for (auto value: values) {
//             std::cout << value.ToString() << " ";
//         }
//         std::cout << "\n";
//     }
//
//     std::cout << "-----------------------\n";
//     std::cout << "SCHEMA:\n";
//     std::cout << schema_.ToString() << "\n";
//
//     std::cout << "=======================\n";
// }

std::vector<CanonicalOD> Fastod::Discover() noexcept {
    Initialize();

    while (!context_in_each_level_[level_].empty()) {
        //std::cout << "Current level: " << level_ << std::endl;
        ComputeODs();

        if (IsTimeUp()) {
            break;
        }

        PruneLevels();
        CalculateNextLevel();

        if (IsTimeUp()) {
            break;
        }

        level_++;
    }

    timer_.Stop();

    if (IsComplete()) {
        std::cout << "FastOD finished successfully" << std::endl;
    } else {
        std::cout << "FastOD finished with a time-out" << std::endl;
    }

    std::cout << "Seconds elapsed: " << timer_.GetElapsedSeconds() << std::endl;
    std::cout << "ODs found: " << fd_count_ + ocd_count_ << std::endl;
    std::cout << "FDs found: " << fd_count_ << std::endl;
    std::cout << "OCDs found: " << ocd_count_ << std::endl;

    return result_;
}

void Fastod::ComputeODs() noexcept {
    auto context_this_level = context_in_each_level_[level_];

    for (auto context: context_this_level) {
        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        auto context_cc = schema_;
        for (auto attribute: context) {
            context_cc = context_cc.Intersect(CCGet(context.DeleteAttribute(attribute)));
        }

        CCPut(context, context_cc);

        if (level_ == 2) {
            for (int i = 0; i < data_.GetColumnCount(); i++) {
                for (int j = 0; j < data_.GetColumnCount(); j++) {
                    if (i == j) {
                        continue;
                    }

                    std::vector<int> t = {i, j};
                    AttributeSet c(t);

                    CSPut(c, AttributePair(SingleAttributePredicate::GetInstance(i, Operator(OperatorType::GreaterOrEqual)), j));
                    CSPut(c, AttributePair(SingleAttributePredicate::GetInstance(i, Operator(OperatorType::LessOrEqual)), j));
                }
            }
        } else  if (level_ > 2) {
            std::unordered_set<AttributePair> candidate_cs_pair_set;

            for (auto attribute: context) {
                auto cs = CSGet(context.DeleteAttribute(attribute));

                for (auto pair: cs) {
                    candidate_cs_pair_set.insert(pair);
                }
            }

            for (auto attribute_pair: candidate_cs_pair_set) {
                auto context_delete_ab = context
                    .DeleteAttribute(attribute_pair.GetLeft().GetAttribute())
                    .DeleteAttribute(attribute_pair.GetRight());

                bool add_context = true;

                for (auto attribute: context_delete_ab) {
                    auto cs = CSGet(context.DeleteAttribute(attribute));

                    if (cs.find(attribute_pair) == cs.end()) {
                        add_context = false;
                        break;
                    }
                }

                if (add_context) {
                    CSPut(context, attribute_pair);
                }
            }
        }
    }

    for (auto context: context_this_level) {
        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        auto context_intersect_cc_context = context.Intersect(CCGet(context));
        for (auto attribute: context_intersect_cc_context) {
            CanonicalOD od(context.DeleteAttribute(attribute), attribute);

            if (od.IsValid(data_, error_rate_threshold_)) {
                result_.push_back(od);
                fd_count_++;
                CCPut(context, CCGet(context).DeleteAttribute(attribute));

                for (auto i: schema_.Difference(context)) {
                    CCPut(context, CCGet(context).DeleteAttribute(i));
                }

                //PrintStatistics();
            }
        }

        std::vector<AttributePair> attribute_pairs_to_remove;

        for (auto attribute_pair: CSGet(context)) {
            auto a = attribute_pair.GetLeft().GetAttribute();
            auto b = attribute_pair.GetRight();

            if (!CCGet(context.DeleteAttribute(b)).ContainsAttribute(a) ||
                !CCGet(context.DeleteAttribute(a)).ContainsAttribute(b)
            ) {
                attribute_pairs_to_remove.push_back(attribute_pair);
            } else {
                CanonicalOD od(context.DeleteAttribute(a).DeleteAttribute(b), attribute_pair.GetLeft(), b);

                if (od.IsValid(data_, error_rate_threshold_)) {
                    ocd_count_++;
                    result_.push_back(od);
                    attribute_pairs_to_remove.push_back(attribute_pair);
                }

                //PrintStatistics();
            }
        }

        for (auto attribute_pair: attribute_pairs_to_remove) {
            CSGet(context).erase(attribute_pair);
        }
    }
}

void Fastod::PruneLevels() noexcept {
    if (level_ >= 2) {
        std::vector<AttributeSet> nodes_to_remove;

        for (auto attribute_set: context_in_each_level_[level_]) {
            if (CCGet(attribute_set).IsEmpty() && CSGet(attribute_set).empty()) {
                nodes_to_remove.push_back(attribute_set);
            }
        }

        auto contexts = context_in_each_level_[level_];
        for (auto attribute_set: nodes_to_remove) {
            contexts.erase(attribute_set);
        }
    }
}

void Fastod::CalculateNextLevel() noexcept {
    std::map<AttributeSet, std::vector<int>> prefix_blocks;
    std::set<AttributeSet> context_next_level;
    auto context_this_level = context_in_each_level_[level_];

    for (auto attribute_set: context_this_level) {
        for (auto attribute: attribute_set) {
            auto prefix = attribute_set.DeleteAttribute(attribute);

            if (prefix_blocks.find(prefix) == prefix_blocks.end()) {
                prefix_blocks[prefix] = std::vector<int>();
            }

            prefix_blocks[prefix].push_back(attribute);
        }
    }

    for (auto &[prefix, single_attributes] : prefix_blocks) {
        if (IsTimeUp()) {
            is_complete_ = false;
            return;
        }

        if (single_attributes.size() <= 1) {
            continue;
        }

        for (int i = 0; i < single_attributes.size(); i++) {
            for (int j = i + 1; j < single_attributes.size(); j++) {
                bool create_context = true;
                auto candidate = prefix.AddAttribute(single_attributes[i]).AddAttribute(single_attributes[j]);

                for (auto attribute: candidate) {
                    if (context_this_level.find(candidate.DeleteAttribute(attribute)) == context_this_level.end()) {
                        create_context = false;
                        break;
                    }
                }

                if (create_context) {
                    context_next_level.insert(candidate);
                }
            }
        }
    }

    context_in_each_level_.push_back(context_next_level);
}
