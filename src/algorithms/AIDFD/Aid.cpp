#include "Aid.h"

#include <easylogging++.h>

namespace algos {

Aid::Aid(const FDAlgorithm::Config& config)
    : FDAlgorithm(config,
                  {kDefaultPhaseName})
{}

unsigned long long Aid::ExecuteInternal() {
    Initialize();

    auto start_time = std::chrono::system_clock::now();

    BuildClusters();

    CreateNegativeCover();

    InvertNegativeCover();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();
}

void Aid::BuildClusters() {
    if (number_of_tuples_ == 0){
        return;
    }

    for (size_t attr_num = 0; attr_num < number_of_attributes_; ++attr_num) {
        auto& column_values_ = clusters_[attr_num];
        bool is_constant = true;
        size_t first_value = tuples_[0][attr_num];
        for (size_t tuple_num = 0; tuple_num < number_of_tuples_; ++tuple_num) {
            size_t entry_value = tuples_[tuple_num][attr_num];
            if (entry_value != first_value) {
                is_constant = false;
            }

            auto cluster_it = column_values_.find(entry_value);
            if (cluster_it == column_values_.end()) {
                column_values_.insert(std::make_pair(entry_value, std::vector<size_t>()));
                cluster_it = column_values_.find(entry_value);
            }

            Cluster& cluster = cluster_it->second;
            cluster.push_back(tuple_num);
            indices_in_clusters_[attr_num][tuple_num] = cluster.size() - 1;
        }

        if (is_constant) {
            constant_columns_[attr_num] = true;
        }
    }
}

bool Aid::AreTerminationCriteriaMet(size_t iteration_num, double curr_ratio) {
    // TODO: probably add other criteria
    iteration_num %= window_size_;
    sum_ -= prev_ratios_[iteration_num];
    sum_ += curr_ratio;
    prev_ratios_[iteration_num] = curr_ratio;
    double average = sum_ / (double)window_size_;
    if (average < growth_threshold_) {
        return true;
    }

    return false;
}

void Aid::CreateNegativeCover() {
    size_t prev_neg_cover_size = 0;
    for (size_t index = 1; ; ++index) {
        for (size_t tuple_num = 0; tuple_num < number_of_tuples_; ++tuple_num) {
            HandleTuple(tuple_num, index);
        }

        size_t curr_neg_cover_size = neg_cover_.size();
        double curr_ratio = 0;
        if (prev_neg_cover_size == 0){
            curr_ratio = (curr_neg_cover_size == 0)
            ? 0.0
            : 1.0;
        }
        else{
            curr_ratio = (double)curr_neg_cover_size / prev_neg_cover_size - 1;
        }

        if (AreTerminationCriteriaMet(index, curr_ratio)){
            break;
        }

        prev_neg_cover_size = curr_neg_cover_size;
    }
}

void Aid::HandleTuple(size_t tuple_num, size_t iteration_num) {
    for (size_t attr_num = 0; attr_num < number_of_attributes_; ++attr_num) {
        size_t value = tuples_[tuple_num][attr_num];
        Cluster cluster = clusters_[attr_num].at(value);
        size_t index_in_cluster = indices_in_clusters_[attr_num][tuple_num];
        if (iteration_num <= index_in_cluster) {
            size_t another_index_in_cluster =
                GenerateSecondClusterIndex(index_in_cluster, iteration_num);
            size_t another_tuple_num = cluster[another_index_in_cluster];
            auto tuples_agree_set = BuildAgreeSet(tuple_num, another_tuple_num);
            neg_cover_.insert(tuples_agree_set);
        }
    }
}

boost::dynamic_bitset<> Aid::BuildAgreeSet(size_t t1, size_t t2) {
    boost::dynamic_bitset<> equal_attr(number_of_attributes_);
    for (size_t attr_num = 0; attr_num < number_of_attributes_; ++attr_num) {
        if (tuples_[t1][attr_num] == tuples_[t2][attr_num]) {
            equal_attr.set(attr_num);
        }
    }

    return equal_attr;
}

void Aid::Initialize() {
    LoadData();
    this->number_of_tuples_ = tuples_.size();
    this->clusters_ = std::vector<std::unordered_map<size_t, Cluster>>(number_of_attributes_);
    this->indices_in_clusters_ = std::vector<std::vector<size_t>>(
        number_of_attributes_, std::vector<size_t>(number_of_tuples_));
    this->constant_columns_ = boost::dynamic_bitset<>(number_of_attributes_);
    this->prev_ratios_ = std::vector<double>(window_size_, 1.0);
    this->sum_ = (double)window_size_ * 1.0;
}

void Aid::HandleConstantColumns(boost::dynamic_bitset<>& attributes) {
    const RelationalSchema* schema = &(*schema_);
    boost::dynamic_bitset<> empty_set(number_of_attributes_);
    Vertical lhs(schema, empty_set);
    for (size_t attr_num = constant_columns_.find_first();
         attr_num != boost::dynamic_bitset<>::npos;
         attr_num = constant_columns_.find_next(attr_num)) {
        attributes[attr_num] = false;
        Column rhs(schema, column_names_[attr_num], attr_num);
        RegisterFd(lhs, rhs);
    }
}

void Aid::HandleInvalidFd(const boost::dynamic_bitset<>& neg_cover_el, SearchTree& pos_cover_tree,
                          size_t rhs) {
    std::vector<boost::dynamic_bitset<>> subsets;
    pos_cover_tree.ForEachSubset(neg_cover_el, [&subsets](const boost::dynamic_bitset<>& subset){
        subsets.push_back(subset);
    });

    for (const auto& subset : subsets) {
        pos_cover_tree.Remove(subset);
    }

    for (auto& subset : subsets) {
        for (size_t add_attr = 0; add_attr < number_of_attributes_; ++add_attr) {
            if (add_attr == rhs || neg_cover_el[add_attr] || constant_columns_[add_attr]) {
                continue;
            }

            subset[add_attr] = true;
            if (!pos_cover_tree.ContainsAnySubsetOf(subset)) {
                pos_cover_tree.Add(subset);
            }

            subset[add_attr] = false;
        }
    }
}

void Aid::InvertNegativeCover() {
    boost::dynamic_bitset<> attributes(number_of_attributes_);
    attributes.set();
    HandleConstantColumns(attributes);

    std::vector<boost::dynamic_bitset<>> neg_cover_vector;
    neg_cover_vector.insert(neg_cover_vector.end(), neg_cover_.begin(), neg_cover_.end());
    auto comp_by_card = [](const boost::dynamic_bitset<>& lhs, const boost::dynamic_bitset<>& rhs) {
        return lhs.count() > rhs.count();
    };
    std::sort(neg_cover_vector.begin(), neg_cover_vector.end(), comp_by_card);
    auto attr_indices = GetAttributesSortedByFrequency(neg_cover_vector);

    std::vector<size_t> inv_attr_indices(number_of_attributes_);
    for (size_t i = 0; i < number_of_attributes_; ++i) {
        inv_attr_indices[attr_indices[i]] = i;
    }

    this->constant_columns_ = ChangeAttributesOrder(this->constant_columns_, inv_attr_indices);
    attributes = ChangeAttributesOrder(attributes, inv_attr_indices);
    for (auto& neg_cover_el : neg_cover_vector){
        neg_cover_el = ChangeAttributesOrder(neg_cover_el, inv_attr_indices);
    }

    for (size_t rhs = 0; rhs < number_of_attributes_; ++rhs) {
        if (constant_columns_[rhs]) {
            continue;
        }

        attributes[rhs] = false;
        SearchTree pos_cover_tree(attributes);
        for (const auto& neg_cover_el : neg_cover_vector) {
            if (!neg_cover_el[rhs]) {
                HandleInvalidFd(neg_cover_el, pos_cover_tree, rhs);
            }
        }

        size_t real_rhs = attr_indices[rhs];
        std::vector<boost::dynamic_bitset<>> pos_cover_vector;
        pos_cover_tree.ForEach([&pos_cover_vector, &attr_indices]
                               (const boost::dynamic_bitset<>& pos_cover_el){
           pos_cover_vector.push_back(ChangeAttributesOrder(pos_cover_el, attr_indices));
        });

        RegisterFDs(real_rhs, pos_cover_vector);
        attributes[rhs] = true;
    }
}

void Aid::LoadData() {
    // TODO: get rid of copy-paste from FDep
    this->number_of_attributes_ = input_generator_.GetNumberOfColumns();
    if (this->number_of_attributes_ == 0) {
        throw std::runtime_error("Unable to work on empty dataset. Check data file");
    }
    this->column_names_.resize(this->number_of_attributes_);

    this->schema_ = std::make_unique<RelationalSchema>(input_generator_.GetRelationName(), true);

    for (size_t i = 0; i < this->number_of_attributes_; ++i) {
        this->column_names_[i] = input_generator_.GetColumnName(static_cast<int>(i));
        this->schema_->AppendColumn(this->column_names_[i]);
    }

    std::vector<std::string> next_line;
    while (input_generator_.GetHasNext()) {
        next_line = input_generator_.ParseNext();
        if (next_line.empty()) break;
        this->tuples_.emplace_back(std::vector<size_t>(this->number_of_attributes_));
        for (size_t i = 0; i < this->number_of_attributes_; ++i) {
            this->tuples_.back()[i] = std::hash<std::string>{}(next_line[i]);
        }
    }
}

void Aid::RegisterFDs(size_t rhs_attribute,
                      const std::vector<boost::dynamic_bitset<>>& list_of_lhs_attributes) {
    const RelationalSchema* schema = &(*schema_);
    Column rhs(schema, column_names_[rhs_attribute], rhs_attribute);
    for (const auto& lhs_attributes : list_of_lhs_attributes) {
        Vertical lhs(schema, lhs_attributes);
        RegisterFd(lhs, rhs);
    }
}

size_t Aid::GenerateSecondClusterIndex(size_t index_in_cluster, size_t iteration_num) const {
    return (iteration_num * prime_) % index_in_cluster;
}

boost::dynamic_bitset<> Aid::ChangeAttributesOrder(const boost::dynamic_bitset<>& initial_bitset,
                                         const std::vector<size_t>& new_order) {
    size_t bitset_size = initial_bitset.size();
    boost::dynamic_bitset<> modified_bitset(bitset_size);
    for (size_t i = 0; i < bitset_size; ++i){
        if (initial_bitset[i]){
            modified_bitset.set(new_order[i]);
        }
    }

    return modified_bitset;
}

std::vector<size_t> Aid::GetAttributesSortedByFrequency(
    const std::vector<boost::dynamic_bitset<>>& neg_cover_vector) const {
    std::vector<unsigned int> frequency(number_of_attributes_, 0);
    for (auto& bitset : neg_cover_vector) {
        for (size_t attr_num = 0; attr_num < number_of_attributes_; ++attr_num) {
            frequency[attr_num] += bitset[attr_num];
        }
    }

    std::vector<size_t> attr_indices(number_of_attributes_);
    for (size_t attr_num = 0; attr_num < number_of_attributes_; ++attr_num) {
        attr_indices[attr_num] = attr_num;
    }

    sort(attr_indices.begin(), attr_indices.end(),
         [&frequency](size_t lhs, size_t rhs) { return frequency[lhs] > frequency[rhs]; });

    return attr_indices;
}
} //namespace algos