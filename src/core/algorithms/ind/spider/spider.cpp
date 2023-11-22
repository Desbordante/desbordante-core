/** \file
 * \brief Spider algorithm
 *
 * Spider algorithm class methods definition
 */
#include "spider.h"

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <type_traits>
#include <utility>

#include "attribute.h"
#include "config/equal_nulls/option.h"
#include "config/error/option.h"
#include "config/mem_limit/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/thread_number/option.h"
#include "util/timed_invoke.h"

namespace algos {

using AttributeIndex = spider::AttributeIndex;

Spider::Spider() : INDAlgorithm({}) {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
    RegisterOption(config::ThreadNumberOpt(&threads_num_));
    RegisterOption(config::MemLimitMBOpt(&mem_limit_mb_));
    RegisterOption(config::ErrorOpt(&max_ind_error_));
    MakeLoadOptsAvailable();
}

void Spider::MakeLoadOptsAvailable() {
    MakeOptionsAvailable({config::EqualNullsOpt.GetName(), config::ThreadNumberOpt.GetName(),
                          config::MemLimitMBOpt.GetName()});
}

void Spider::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::ErrorOpt.GetName()});
}

void Spider::LoadDataInternal() {
    auto const create_domains = [&] {
        domains_ = model::ColumnDomain::CreateFrom(input_tables_, mem_limit_mb_, threads_num_);
    };
    timings_.load = util::TimedInvoke(create_domains);
}

namespace {
template <typename Attribute>
std::vector<Attribute> InitAttributes(std::vector<model::ColumnDomain> const& domains) {
    std::vector<Attribute> attrs;
    AttributeIndex attr_count = domains.size();
    attrs.reserve(attr_count);
    for (AttributeIndex attr_id = 0; attr_id != attr_count; ++attr_id) {
        attrs.emplace_back(attr_id, attr_count, domains[attr_id]);
    }
    return attrs;
}

template <typename Attribute>
std::vector<Attribute> GetProcessedAttributes(std::vector<model::ColumnDomain> const& domains,
                                              config::EqNullsType is_null_equal_null) {
    using AttributeRW = std::reference_wrapper<Attribute>;
    std::vector attrs = InitAttributes<Attribute>(domains);
    std::priority_queue<AttributeRW, std::vector<AttributeRW>, std::greater<Attribute>> attr_pq(
            attrs.begin(), attrs.end());
    boost::dynamic_bitset<> ids_bitset(attrs.size());
    while (!attr_pq.empty()) {
        AttributeRW attr_rw = attr_pq.top();
        std::string const& value = attr_rw.get().GetCurrentValue();
        do {
            attr_pq.pop();
            ids_bitset.set(attr_rw.get().GetId());
            if (attr_pq.empty()) break;
            attr_rw = attr_pq.top();
            if (value.empty() && !is_null_equal_null) break;
        } while (attr_rw.get().GetCurrentValue() == value);

        auto ids_vec = util::BitsetToIndices<AttributeIndex>(ids_bitset);
        for (auto id : ids_vec) {
            if constexpr (std::is_same_v<Attribute, spider::INDAttribute>) {
                attrs[id].IntersectRefs(ids_bitset, attrs);
            } else {
                attrs[id].IntersectRefs(ids_vec);
            }
        }
        for (auto id : ids_vec) {
            Attribute& attr = attrs[id];
            if (!attr.HasFinished()) {
                attr.MoveToNext();
                attr_pq.emplace(attr);
            }
        }
        ids_bitset.reset();
    }
    return attrs;
}
};  // namespace

void Spider::RegisterIND(model::ColumnCombination lhs, model::ColumnCombination rhs) {
    RegisterIND(std::make_shared<model::ColumnCombination>(std::move(lhs)),
                std::make_shared<model::ColumnCombination>(std::move(rhs)));
}

void Spider::MineINDs() {
    using spider::INDAttribute;
    std::vector const attrs = GetProcessedAttributes<INDAttribute>(domains_, is_null_equal_null_);
    for (auto const& dep : attrs) {
        for (AttributeIndex ref_id : dep.GetRefIds()) {
            RegisterIND(dep.ToCC(), attrs[ref_id].ToCC());
        }
    }
}

void Spider::MineAINDs() {
    using spider::AINDAttribute;
    std::vector const attrs = GetProcessedAttributes<AINDAttribute>(domains_, is_null_equal_null_);
    for (auto const& dep : attrs) {
        for (AttributeIndex ref_id : dep.GetRefIds(max_ind_error_)) {
            RegisterIND(dep.ToCC(), attrs[ref_id].ToCC());
        }
    }
}

unsigned long long Spider::ExecuteInternal() {
    auto const mining_func = (max_ind_error_ == 0) ? &Spider::MineINDs : &Spider::MineAINDs;
    timings_.compute = util::TimedInvoke(mining_func, this);
    timings_.total = timings_.load + timings_.compute;
    return timings_.total;
}

void Spider::ResetINDAlgorithmState() {
    timings_.compute = 0;
    timings_.total = 0;
}

}  // namespace algos
