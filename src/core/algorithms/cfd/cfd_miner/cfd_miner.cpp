#include "cfd_miner.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <utility>

#include "core/config/exceptions.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "gc_growth.h"

namespace algos::cfd {

CFDMiner::CFDMiner() {
    RegisterOptions();
}

void CFDMiner::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto check_support = [this](unsigned support) {
        if (support == 0 || support > relation_->GetNumRows()) {
            throw config::ConfigurationError(
                    "Minimum support must be not less than 1 and no more than the number of "
                    "tuples.");
        }
    };
    auto check_lhs = [](unsigned max_lhs) {
        if (max_lhs == 0) {
            throw config::ConfigurationError("Maximum LHS size must be greater than 0.");
        }
    };

    RegisterOption(Option{&min_support_, kCfdMinimumSupport, kDCfdMinimumSupport, 1u}.SetValueCheck(
            std::move(check_support)));
    RegisterOption(
            Option{&max_lhs_, kCfdMaximumLhs, kDCfdMaximumLhs, std::numeric_limits<unsigned>::max()}
                    .SetValueCheck(std::move(check_lhs)));
}

void CFDMiner::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kCfdMinimumSupport, kCfdMaximumLhs});
}

void CFDMiner::ResetStateCFD() {}

void CFDMiner::ExecuteInternal() {
    std::vector<Transaction> transactions;
    transactions.reserve(relation_->GetNumRows());
    for (std::size_t row = 0; row < relation_->GetNumRows(); ++row) {
        transactions.push_back(relation_->GetRow(row));
    }

    GCGrowth miner;
    auto closed_to_free = miner.Mine(transactions, min_support_, max_lhs_);
    std::map<Itemset, Itemset const*> generator_to_closure;
    for (auto const& [closure, generators] : closed_to_free) {
        for (auto const& generator : generators) {
            generator_to_closure.emplace(generator.items, &closure);
        }
    }

    for (auto const& [lhs, closure] : generator_to_closure) {
        Itemset rhs;
        std::set_difference(closure->begin(), closure->end(), lhs.begin(), lhs.end(),
                            std::back_inserter(rhs));

        for (std::size_t index = 0; index < lhs.size() && !rhs.empty(); ++index) {
            auto subset = lhs;
            subset.erase(subset.begin() + index);
            auto const& subset_closure = *generator_to_closure.at(subset);
            Itemset minimal_rhs;
            std::set_difference(rhs.begin(), rhs.end(), subset_closure.begin(),
                                subset_closure.end(), std::back_inserter(minimal_rhs));
            rhs = std::move(minimal_rhs);
        }

        for (Item item : rhs) {
            RegisterCfd(lhs, item);
        }
    }
}

void CFDMiner::RegisterCfd(Itemset const& lhs, Item rhs) {
    auto make_raw_item = [this](Item item) {
        return RawCFD::RawItem{.attribute = relation_->GetAttrIndex(item),
                               .value = std::optional<std::string>{relation_->GetValue(item)}};
    };

    RawCFD::RawItems raw_lhs;
    std::transform(lhs.begin(), lhs.end(), std::back_inserter(raw_lhs), make_raw_item);
    std::sort(raw_lhs.begin(), raw_lhs.end(),
              [](auto const& left, auto const& right) { return left.attribute < right.attribute; });

    cfd_list_.emplace_back(std::move(raw_lhs), make_raw_item(rhs));
}

}  // namespace algos::cfd
