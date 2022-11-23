#include "cfd_discovery.h"

// see ./LICENSE

#include <iterator>
#include <thread>

#include "algorithms/algo_factory.h"
#include "algorithms/CFD/partition_table.h"
#include "util/set_util.h"
#include "util/cfd_output_util.h"

namespace algos {

decltype(CFDDiscovery::MinSupportOpt) CFDDiscovery::MinSupportOpt{
        {config::names::kCfdMinimumSupport, config::descriptions::kDCfdMinimumSupport}, 0};

decltype(CFDDiscovery::ColumnsNumberOpt) CFDDiscovery::ColumnsNumberOpt{
        {config::names::kCfdColumnsNumber, config::descriptions::kDCfdColumnsNumber}, 0};

decltype(CFDDiscovery::TuplesNumberOpt) CFDDiscovery::TuplesNumberOpt{
        {config::names::kCfdTuplesNumber, config::descriptions::kDCfdTuplesNumber}, 0};

decltype(CFDDiscovery::MinConfidenceOpt) CFDDiscovery::MinConfidenceOpt{
        {config::names::kCfdMinimumConfidence, config::descriptions::kDCfdMinimumConfidence}, 0.0};

decltype(CFDDiscovery::MaxLhsSizeOpt) CFDDiscovery::MaxLhsSizeOpt{
        {config::names::kCfdMaximumLhs, config::descriptions::kDCfdMaximumLhs}, 0};

/* decltype(CFDDiscovery::AlgoStrOpt) CFDDiscovery::AlgoStrOpt{
        {"primitive", "algorithm to use for data profiling\n" +
                              EnumToAvailableValues<algos::PrimitiveType>() + " + [ac]"}}; */

CFDDiscovery::CFDDiscovery(std::vector<std::string_view> phase_names)
    : Primitive(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(config::EqualNullsOpt, TuplesNumberOpt, ColumnsNumberOpt));
}

CFDDiscovery::CFDDiscovery() : CFDDiscovery({kDefaultPhaseName}) {}

void CFDDiscovery::FitInternal(model::IDatasetStream& data_stream) {
    if (relation_ == nullptr) {
        relation_ =
                CFDRelationData::CreateFrom(data_stream, is_null_equal_null_,
                                            columns_number_, tuples_number_);
    }

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty .csv file: FD mining is meaningless.");
    }
}

void CFDDiscovery::RegisterOptions() {
    RegisterOption(config::EqualNullsOpt.GetOption(&is_null_equal_null_));
    RegisterOption(MinSupportOpt.GetOption(&min_supp_));
    RegisterOption(MinConfidenceOpt.GetOption(&min_conf_));
    RegisterOption(TuplesNumberOpt.GetOption(&tuples_number_));
    RegisterOption(ColumnsNumberOpt.GetOption(&columns_number_));
    RegisterOption(MaxLhsSizeOpt.GetOption(&max_lhs_));
    //RegisterOption(AlgoStrOpt.GetOption(&algo_name_));
}

void CFDDiscovery::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable(config::GetOptionNames(MinSupportOpt, MinConfidenceOpt, MaxLhsSizeOpt));
}

unsigned long long CFDDiscovery::ExecuteInternal()
{
    max_cfd_size_ = max_lhs_ + 1;
    auto start_time = std::chrono::system_clock::now();
    //if (algo_name_ == "fd_first_dfs_dfs") {
        FdsFirstDFS(min_supp_, max_lhs_ + 1, kDfs, min_conf_);
    //}
    //else if (algo_name_ == "fd_first_dfs_bfs") {
        //FdsFirstDFS(min_supp_, max_lhs_ + 1, kBfs, min_conf_);
    //}
    /* LOG(INFO) << generators_.GetGenMap().size() << " " << store_.size() << ' ' << tid_store_.size()
        << ' ' << all_attrs_.size() << ' ' << items_layer_.size() << ' ' << free_map_.size() << free_itemsets_.size() << ' ' << ' ' << rules_.size() << ' ' << attribute_order_.size(); */
    /* else if (config_.algo_choice == "FD-First-kBfs-bfs" || config_.algo_choice == "FD-First-kBfs-bfs") {
        FdsFirstBFS(config_.min_supp, config_.max_cfd_size, config_.substrategy, config_.min_conf);
    }
    else if (config_.algo_choice == "Itemset-First-kDfs-dfs" || config_.algo_choice == "Itemset-First-kDfs-bfs") {
        ItemsetsFirstDFS(config_.min_supp, config_.max_cfd_size, config_.substrategy, config_.min_conf);
    }
    else if (config_.algo_choice == "Itemset-First-kBfs-dfs" || config_.algo_choice == "Itemset-First-kBfs-bfs") {
        ItemsetsFirstBFS(config_.min_supp, config_.max_cfd_size, config_.substrategy, config_.min_conf);
    }
    else if (config_.algo_choice == "Integrated-kDfs") {
        IntegratedDFS(config_.min_supp, config_.max_cfd_size, config_.min_conf);
    }
    else if (config_.algo_choice == "Integrated-kBfs") {
        Ctane(config_.min_supp, config_.max_cfd_size, config_.min_conf);
    } */

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    long long apriori_millis = elapsed_milliseconds.count();
    LOG(INFO) << "> CFD COUNT: " << cfd_list_.size();

    return apriori_millis;
}

[[maybe_unused]] int CFDDiscovery::NrCfds() const {
    return (int)cfd_list_.size();
}

CFDList CFDDiscovery::GetCfds() const {
    return cfd_list_;
}

std::string CFDDiscovery::GetCfdString(CFD const& cfd) const {
    return Output::CFDToString(cfd, relation_);
}

std::string CFDDiscovery::GetRelationString(char delim) const {
    return relation_->GetStringFormat(delim);
}

std::string CFDDiscovery::GetRelationString(const SimpleTidList& subset, char delim) const {
    return relation_->GetStringFormat(subset, delim);
}

bool CFDDiscovery::Precedes(const Itemset& a, const Itemset& b) {
    if (a.size() > b.size()) return false;
    if (a == b) return false;
    Itemset pattern(relation_->GetAttrsNumber());
    for (int v : b) {
        if (v > 0) pattern[relation_->GetAttrIndex(v)] = v;
        else pattern[-1-v] = v;
    }
    for (int i : a) {
        if (i > 0) {
            int bv = pattern[relation_->GetAttrIndex(i)];
            if (bv != i) {
                return false;
            }
        }
        else {
            if (pattern[-1-i] == 0) return false;
        }
    }
    return true;
}

// Проверяем если у на в items во всех классах эквивалентности одно и то же значение в атрибуте rhs_a
// Проверяем по одному значению из каждого класса эквивалентности.
bool CFDDiscovery::IsConstRule(const PartitionTidList& items, int rhs_a) {
    int rhs_value;
    bool first = true;
    for (unsigned pos_index = 0; pos_index <= items.tids.size(); pos_index++) {
        if (pos_index == items.tids.size() || items.tids[pos_index] == PartitionTidList::SEP) {
            auto tup = relation_->GetRow(items.tids[pos_index - 1]);
            if (first) {
                first = false;
                rhs_value = tup[rhs_a];
            }
            else {
                if (tup[rhs_a] != rhs_value) return false;
            }
        }
    }
    return true;
}

// Вроде просто всё проинициализировали.
void CFDDiscovery::FdsFirstDFS(int min_supp, unsigned max_lhs_size, SUBSTRATEGY ss, double min_conf) {
    min_supp_ = min_supp;
    min_conf_ = min_conf;
    max_cfd_size_ = max_lhs_size;
    all_attrs_ = range(-(int)relation_->GetAttrsNumber(),
                       0); // Я так понимаю, что тут sep = 0. Возвращается список от -FBD. GetAttrsNumber() до 0 включительно
    PartitionTable::database_row_number_ = (int)relation_->size();
    std::vector<MinerNode<PartitionTidList> > items = GetPartitionSingletons();
    // cands это изначально список всех атрибутов.
    for (auto& a : items) {
        a.cands = all_attrs_;
        // Тут по паре вида (support, sets_number) кодируется Itemset - [a.item] (отрицат) item это вроде закодированный номер атрибута
        free_map_[std::make_pair(support(a.tids),a.tids.sets_number)].push_back(itemset(a.item));
        free_itemsets_.insert(itemset(a.item));
    }
    cand_store_ = PrefixTree<Itemset, Itemset>();
    // По ключу в виде пустого itemset'a получаем список вида 0 1 2 ... размера FBD.size()
    store_[Itemset()] = convert(iota((int)relation_->size()));
    cand_store_.insert(Itemset(), all_attrs_);
    FdsFirstDFS(Itemset(), items, ss);
}

// В первый раз передаётся пустой itemset и items из предыдущ. функции. Идём по каждому элементу inode из items с конца.
void CFDDiscovery::FdsFirstDFS(const Itemset &prefix, std::vector<MinerNode<PartitionTidList> > &items, SUBSTRATEGY ss) {
    for (int ix = items.size()-1; ix >= 0; ix--) {
        MinerNode<PartitionTidList>& inode = items[ix];
        // Вставляет элемент inode.item в prefix, чтобы всё было отсортировано.
        const Itemset iset = join(prefix, inode.item);

        // Просто пересечение двух отсортированных множеств/списков
        auto insect = intersection(iset, inode.cands);
        for (int out : insect) {
            // Содержит все элементы iset, не включая равные out. То есть по сути тут реализована разность двух множеств
            Itemset sub = subset(iset, out);
            // Если у нас всего один класс эквивалентности в inode
            // Или же в атрибуте с индексом (-1 - out) во всех класса одинаковые значения, то мы скипаем этот проход цикла
            if (inode.tids.sets_number == 1 || IsConstRule(inode.tids, -1 - out)) continue;
            // find возвращает итератор (примерно указатель) на найденный элемент
            auto stored_sub = store_.find(sub);
            if (stored_sub == store_.end()) {
                continue;
            }
            bool lhs_gen = true;
            if (free_itemsets_.find(sub) == free_itemsets_.end()) {
                lhs_gen = false;
            }
            if (rules_.find(out) != rules_.end()) {
                for (const auto& sub_rule : rules_[out]) {
                    if (!has(sub_rule, [](int si) -> bool { return si < 0; })) continue;
                    if (Precedes(sub_rule, sub)) {
                        lhs_gen = false;
                    }
                }
            }
            if (lhs_gen) {
                // В e содержится вот эта сумма всех ошибок из статьи
                double e = PartitionTable::PartitionError(stored_sub->second, inode.tids);
                // Считается по формуле из статьи
                double conf = 1 - (e / support(stored_sub->second));
                // Если conf нас удовлетворяет, то мы в cfd_list_ добавляем новую зависимость
                // вида sub -> out, то есть вида I \ {j} -> j, как в статье.
                if (conf >= min_conf_) {
                    cfd_list_.emplace_back(sub, out);
                }
                // Если у нас conf вообще супер, то мы добавляем sub в словарь rules_ под индексом out
                if (conf >= 1) {
                    rules_[out].push_back(sub);
                }
            }
            // Дальше вызываем minePatterns в конце цикла по элементам пересечения.
            // То есть мы уже намайнили fd. И сейчас будем майнить паттерны к ним.
            if (ss == SUBSTRATEGY::kDfs)
                MinePatternsDFS(sub, out, inode.tids);
            /* else if (ss == SUBSTRATEGY::kBfs)
                 MinePatternsBFS(sub, out, inode.tids); */
        }
        if (inode.cands.empty()) continue;
        if (iset.size() == max_cfd_size_) continue;
        store_[iset] = inode.tids;
        cand_store_.insert(iset, inode.cands);
        // Получаем сисок атрибутов элементов iset
        // Не в этом проблема
        auto node_attrs = relation_->GetAttrVector(iset);
        std::vector<const PartitionTidList*> expands;
        std::vector<MinerNode<PartitionTidList> > tmp_suffix;
        // Идём по каждому MinerNode'у. Если у нас находиться атрибут его элемента в node_attrs, то мы скипаем цикл. Иначе идём дальше
        // Создаём новое множество newset, которое состоит из iset + jnode.item.
        // Заводим ещё мн-во с, как пересечение inode.cands и jnode.cands
        // Дальше в newset рассматриваем zsub - newset без какого-то элемента.
        // Если находим его в store_, то ещё находим subCands и делаем c = intersection(c, subCands);
        // Мы пересекаем c и subcands много раз. Если с непустое, то мы делаем разные добавления.
        for (int jx = (int)items.size()-1; jx > ix; jx--) {
            const auto &jnode = items[jx];
            if (std::binary_search(node_attrs.begin(), node_attrs.end(),
                                   relation_->GetAttrIndex(jnode.item))) continue;
            Itemset newset = join(iset, jnode.item);
            auto c = intersection(inode.cands, jnode.cands);
            for (int zz : newset) {
                auto zsub = subset(newset, zz);
                auto stored_sub = store_.find(zsub);
                if (stored_sub == store_.end()) {
                    c.clear();
                    break;
                }
                const Itemset & sub_cands = *cand_store_.find(zsub);
                c = intersection(c, sub_cands);
            }
            if (c.size()) {
                expands.push_back(&items[jx].tids);
                tmp_suffix.emplace_back(items[jx].item);
                tmp_suffix.back().cands = c;
                // Тут tmp_suffix.back().item вроде = items[jx].item
                tmp_suffix.back().prefix = subset(newset, tmp_suffix.back().item);
            }
        }
        const auto exps = PartitionTable::intersection(items[ix].tids, expands);
        std::vector<MinerNode<PartitionTidList> > suffix;

        for (unsigned e = 0; e < exps.size(); e++) {
            bool gen = true;
            auto new_set = join(tmp_suffix[e].prefix, tmp_suffix[e].item);
            auto sp = std::make_pair(support(exps[e]), exps[e].sets_number);
            auto free_map_elem = free_map_.find(sp);
            if (free_map_elem != free_map_.end()) {
                auto free_cands = free_map_elem->second;
                for (const auto &sub_cand : free_cands) {
                    if (IsSubsetOf(sub_cand, new_set)) {
                        gen = false;
                        break;
                    }
                }
            }
            if (gen) {
                free_map_[sp].push_back(new_set);
                free_itemsets_.insert(new_set);
            }
            suffix.emplace_back(tmp_suffix[e].item, exps[e]);
            suffix.back().cands = tmp_suffix[e].cands;
            suffix.back().prefix = tmp_suffix[e].prefix;
        }
        if (suffix.size()) {
            std::sort(suffix.begin(), suffix.end(), [](const MinerNode<PartitionTidList>& a, const MinerNode<PartitionTidList>& b) {
                return a.tids.sets_number < b.tids.sets_number;
            });
            FdsFirstDFS(iset, suffix, ss);
        }
    }
}



int CFDDiscovery::GetPartitionSupport(const SimpleTidList& pids, const std::vector<int>& psupps) {
    int res = 0;
    for (int p : pids) {
        res += psupps[p];
    }
    return res;
}

int CFDDiscovery::GetPartitionError(const SimpleTidList& pids, const std::vector<std::pair<Itemset, std::vector<int> > >& partitions) {
    int res = 0;
    for (int p : pids) {
        int max = 0;
        int total = 0;
        for (const auto& rhs : partitions[p].second) {
            total += rhs;
            if (rhs > max) {
                max = rhs;
            }
        }
        res += total - max;
    }
    return res;
}

bool CFDDiscovery::IsConstRulePartition(const SimpleTidList &items,
                                        const std::vector<std::vector<std::pair<int, int> > > &rhses) {
    int rhs_value;
    bool first = true;
    for (int pi : items) {
        if (first) {
            first = false;
            rhs_value = rhses[pi][0].first;
        }
        for (auto rp : rhses[pi]) {
            int r = rp.first;
            if (r != rhs_value) return false;
        }
    }
    return true;
}

void CFDDiscovery::MinePatternsDFS(const Itemset &lhs, int rhs, const PartitionTidList & all_tids) {
    std::map<int, SimpleTidList> pid_lists;
    auto lhs_attrs = relation_->GetAttrVector(lhs);
    std::vector<std::pair<Itemset, std::vector<int> > > partitions;
    std::vector<std::vector<std::pair<int,int> > > pair_rhses;
    std::unordered_map<Itemset, int> rule_ixs;
    std::vector<int> rhses;
    int count = 0;
    for (unsigned pi = 0; pi <= all_tids.tids.size(); pi++) {
        if (pi == all_tids.tids.size() || all_tids.tids[pi] == PartitionTidList::SEP ) {
            const Transaction& trans = relation_->GetRow(all_tids.tids[pi - 1]);
            std::vector<int> lhs_constants = projection(trans, lhs_attrs);
            auto rule_i = rule_ixs.find(lhs_constants);
            if (rule_i == rule_ixs.end()) {
                rhses.push_back(trans[-1-rhs]);
                pair_rhses.emplace_back();
                pair_rhses.back().emplace_back(trans[-1-rhs],count);
                rule_ixs[lhs_constants] = (int)partitions.size();
                partitions.emplace_back(lhs_constants, std::vector<int>());
                partitions.back().second.push_back(count);
            }
            else {
                partitions[rule_i->second].second.push_back(count);
                pair_rhses[rule_i->second].emplace_back(trans[-1-rhs], count);
            }
            count = 0;
        }
        else {
            count++;
        }
    }
    std::vector<MinerNode<SimpleTidList> > items;
    std::vector<int> psupps(partitions.size());
    int ri = 0;
    for (const auto& rule : partitions) {
        for (int i : rule.first) {
            pid_lists[i].push_back(ri);
        }
        int s = 0;
        for (const auto& rhs_items : rule.second) {
            s += rhs_items;
        }
        psupps[ri] = s;
        ri++;
    }
    for (const auto& item : pid_lists) {
        int psupp = GetPartitionSupport(item.second, psupps);
        if (psupp >= min_supp_) {
            Itemset ns = join(itemset(item.first), subset(lhs, -1- relation_->GetAttrIndex(item.first)));
            std::set<Itemset> nr_parts;
            for (int pid : item.second) {
                nr_parts.insert(partitions[pid].first);
            }
            bool gen = true;
            auto sp = std::make_pair(psupp, nr_parts.size());
            auto free_map_pair = free_map_.find(sp);
            if (free_map_pair != free_map_.end()) {
                auto free_cands = free_map_pair->second;
                for (const auto &sub_cand : free_cands) {
                    if (IsSubsetOf(sub_cand, ns)) {
                        gen = false;
                        break;
                    }
                }
            }
            if (gen) {
                free_map_[sp].push_back(ns);
                free_itemsets_.insert(ns);
            }
            items.emplace_back(item.first, item.second, psupp);
        }
    }
    MinePatternsDFS(Itemset(), items, lhs, rhs, pair_rhses, partitions, psupps);
}

void CFDDiscovery::MinePatternsDFS(const Itemset &prefix, std::vector<MinerNode<SimpleTidList> > &items,
                                   const Itemset &lhs, int rhs,
                                   std::vector<std::vector<std::pair<int, int> > > & rhses_pair,
                                   std::vector<std::pair<Itemset, std::vector<int> > > &partitions,
                                   std::vector<int> &psupps) {
    for (int ix = items.size()-1; ix >= 0; ix--) {
        const auto &inode = items[ix];
        Itemset iset = join(prefix, inode.item);
        auto node_attrs = relation_->GetAttrVectorItems(iset);
        bool lhs_gen = true;
        int out = (iset.size() == lhs.size()) ? GetMaxElem(rhses_pair[inode.tids[0]]) : rhs;
        auto sub = join(iset, SetDiff(lhs, node_attrs));

        if (out > 0 || !IsConstRulePartition(inode.tids, rhses_pair)) {
            if (rules_.find(out) != rules_.end()) {
                for (const auto & sub_rule : rules_[out]) {
                    if (out < 0 && !has(sub_rule, [](int si) -> bool { return si < 0; })) continue;
                    if (Precedes(sub_rule, sub)) {
                        lhs_gen = false;
                    }
                }
            }
            if (lhs_gen) {
                int e = GetPartitionError(inode.tids, partitions);
                double conf = 1.0 - ((double) e / (double) inode.node_supp);
                if (conf >= min_conf_) {
                    cfd_list_.emplace_back(sub, out);
                }
                if (conf >= 1) {
                    rules_[out].push_back(sub);
                    if (out > 0) rules_[-1 - relation_->GetAttrIndex(out)].push_back(sub);
                }
            }
        }
        std::vector<MinerNode<SimpleTidList> > suffix;
        for (unsigned j = ix + 1; j < items.size(); j++) {
            const auto &jnode = items[j];
            if (std::binary_search(node_attrs.begin(), node_attrs.end(), -1- relation_->GetAttrIndex(jnode.item))) continue;
            Itemset newset = join(iset, jnode.item);
            SimpleTidList ij_tids = intersection(inode.tids, jnode.tids);
            int ij_supp = GetPartitionSupport(ij_tids, psupps);
            if (ij_supp >= min_supp_){
                bool gen = true;
                auto nas = relation_->GetAttrVectorItems(newset);
                Itemset ns = join(newset, SetDiff(lhs, nas));
                std::set<Itemset> nr_parts;
                for (int pid : ij_tids) {
                    nr_parts.insert(partitions[pid].first);
                }
                auto sp = std::make_pair(ij_supp, nr_parts.size());
                auto free_map_pair = free_map_.find(sp);
                if (free_map_pair != free_map_.end()) {
                    auto free_cands = free_map_pair->second;
                    for (const auto &sub_cand : free_cands) {
                        if (IsSubsetOf(sub_cand, ns)) {
                            gen = false;
                            break;
                        }
                    }
                }
                if (gen) {
                    free_map_[sp].push_back(ns);
                    free_itemsets_.insert(ns);
                }
                suffix.emplace_back(jnode.item, std::move(ij_tids), ij_supp);
            }
        }
        if (suffix.size()) {
            std::sort(suffix.begin(), suffix.end(), [](const MinerNode<SimpleTidList>& a, const MinerNode<SimpleTidList>& b) {
                return support(a.tids) < support(b.tids);
            });
            MinePatternsDFS(iset, suffix, lhs, rhs, rhses_pair, partitions, psupps);
        }
    }
}
// Тут смысл вроде получить всевозможные синглтоны через список значений типа MinerNode<PartitionTidList>
std::vector<MinerNode<PartitionTidList> > CFDDiscovery::GetPartitionSingletons() {
    std::vector<std::vector<SimpleTidList> > partitions(relation_->GetAttrsNumber());
    std::unordered_map<int, std::pair<int,int> > attr_indices;
    for (unsigned a = 0; a < relation_->GetAttrsNumber(); a++) {
        const auto& dom = relation_->GetDomain(a);
        partitions[a] = std::vector<SimpleTidList>(dom.size());
        for (unsigned i = 0; i < dom.size(); i++) {
            partitions[a][i].reserve(relation_->Frequency(dom[i]));
            attr_indices[dom[i]] = std::make_pair(a, i);
        }
    }
    // attr_indices содержит словарь, где ключ - значение какого-то элемента в его домене, а value
    // это пара из атрибута этого домена и индекса этого значения.
    for (unsigned row = 0; row < relation_->size(); row++) {
        const auto& tup = relation_->GetRow(row);
        for (int item : tup) {
            const auto& attr_node_ix = attr_indices.at(item);
            partitions[attr_node_ix.first][attr_node_ix.second].push_back(row);
        }
    }
    // Partitions это трёхмерный список. В partitions[a][i] содержатся индексы кортежей в бд
    // в которых в атрибуте с индексом a хранится значение a.dom[i] (то есть i-ый элемент домена атрибута)
    // Почему attrItem = -1 - a. Шо это значит???
    // Тут он заполняет вектор типа std::vector<MinerNode<PartitionTidList> > всеми значениями из partitions, как я понял
    // Потом в конце каждого цикла (цикла по атрибутам) он хэширует получившийся Partitiontidlist.
    // attrItem это видимо закодированный номер атрибута???
    std::vector<MinerNode<PartitionTidList> > singletons;
    for (unsigned a = 0; a < relation_->GetAttrsNumber(); a++) {
        int attr_item = -1 - a;
        singletons.emplace_back(attr_item);
        const auto& dom = relation_->GetDomain(a);
        singletons.back().tids.tids.reserve(relation_->size()+dom.size()-1);
        singletons.back().tids.sets_number = dom.size();
        for (unsigned i = 0; i < dom.size(); i++) {
            auto& ts = singletons.back().tids.tids;
            ts.insert(ts.end(), partitions[a][i].begin(), partitions[a][i].end());
            if (i < dom.size() - 1) {
                ts.push_back(PartitionTidList::SEP);
            }
        }
        singletons.back().HashTids();
    }
    return singletons;
}

// Константный аналог предыдущего метода
std::vector<MinerNode<PartitionTidList> > CFDDiscovery::GetPartitionSingletons(const SimpleTidList& tids, const Itemset& attrs) {
    std::vector<std::vector<SimpleTidList> > partitions(relation_->GetAttrsNumber());
    std::unordered_map<int, std::pair<int,int> > attr_indices;
    for (int a : attrs) {
        const auto& dom = relation_->GetDomain(a);
        partitions[a] = std::vector<SimpleTidList>(dom.size());
        for (unsigned i = 0; i < dom.size(); i++) {
            //partitions[a][i].reserve(FBD.Frequency(dom[i]));
            attr_indices[dom[i]] = std::make_pair(a, i);
        }
    }
    for (int row: tids) {
        const auto& tup = relation_->GetRow(row);
        for (int a : attrs) {
            int item = tup[a];
            const auto& attr_node_ix = attr_indices.at(item);
            partitions[attr_node_ix.first][attr_node_ix.second].push_back(row);
        }
    }
    std::vector<MinerNode<PartitionTidList> > singletons;
    for (int a : attrs) {
        int attr_item = -1 - a;
        singletons.emplace_back(attr_item);
        const auto& dom = relation_->GetDomain(a);
        singletons.back().tids.tids.reserve(tids.size()+dom.size()-1);
        singletons.back().tids.sets_number = 0;//dom.size();
        for (unsigned i = 0; i < dom.size(); i++) {
            if (!partitions[a][i].empty()) {
                singletons.back().tids.sets_number++;
                auto &ts = singletons.back().tids.tids;
                ts.insert(ts.end(), partitions[a][i].begin(), partitions[a][i].end());
                ts.push_back(PartitionTidList::SEP);
            }
        }
        singletons.back().tids.tids.pop_back();
        singletons.back().HashTids();
    }
    return singletons;
}


// Тут к parts добавляются singles и возвращаются parts
std::vector<MinerNode<PartitionTidList> > CFDDiscovery::GetAllSingletons(int min_supp) {
    auto parts = GetPartitionSingletons();
    auto singles = GetSingletons(min_supp);
    for (const auto& sing : singles) {
        parts.emplace_back(sing.item);
        parts.back().tids = {sing.tids, 1};
    }
    return parts;
}

// Ищем сначала элементы из items_, которые повторяются (Frequency) >= minsup. Добавляем элементы в singletons
// И добавляем их в словарь nodeIndicies, где ключ - индекс этого элемента в items_, а значение - (singletons.size() - 1)
// Дальше идём по каждому элементу в каждой строчке в FBD. Если такой элемент был добавлен в nodeIndices, то мы берём второе
// значение в паре? (т.е. singletons.size() - 1) и по этому индексу берём tids и добавляем туда индекс строки, которую рассматриваем.
// Дальше записываем в каждом minernode.node_hash с помощью HashTids(). И возвращаем этот список singletons;

// Логика - сначала заполняем в singletons подходящие элементы с достаточным Frequency. Потом мы заполняем у этих элементов tids
// значением row (то есть в tids находятся тиды одинаковых элементов из nodeIndicies) Потом мы хэшируем все Minernode'ы и всё.
std::vector<MinerNode<SimpleTidList> > CFDDiscovery::GetSingletons(int minsup) {
    std::vector<MinerNode<SimpleTidList> > singletons;
    std::unordered_map<int, int> node_indices;
    for (int item = 1; item <= (int)relation_->GetItemsNumber(); item++) {
        if (relation_->Frequency(item) >= minsup) {
            singletons.emplace_back(item, relation_->Frequency(item));
            node_indices[item] = (int)singletons.size() - 1;
        }
    }

    for (unsigned row = 0; row < relation_->size(); row++) {
        const auto& tup = relation_->GetRow((int)row);
        for (int item : tup) {
            const auto& it = node_indices.find(item);
            if (it != node_indices.end()) {
                singletons[it->second].tids.push_back((int)row);
            }
        }
    }

    //std::sort(singletons.begin(), singletons.end());
    for (int i = (int)singletons.size() - 1; i >= 0; i--) {
        auto& node = singletons[i];
        node.HashTids();
    }
    return singletons;
}
}