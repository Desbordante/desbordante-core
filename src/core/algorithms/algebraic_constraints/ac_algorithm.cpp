#include "ac_algorithm.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <random>

#include <easylogging++.h>

#include "config/exceptions.h"
#include "config/names_and_descriptions.h"
#include "config/tabular_data/input_table/option.h"
#include "types/create_type.h"

namespace algos {

ACAlgorithm::ACAlgorithm() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::TableOpt.GetName()});
    ac_exception_finder_ = std::make_unique<algebraic_constraints::ACExceptionFinder>();
}

void ACAlgorithm::RegisterOptions() {
    using namespace config::names;
    using namespace config::descriptions;
    using config::Option;

    auto check_and_set_binop = [this](Binop bin_operation) {
        switch (bin_operation) {
            case +Binop::Addition:
                binop_pointer_ = &model::INumericType::Add;
                break;
            case +Binop::Subtraction:
                binop_pointer_ = &model::INumericType::Sub;
                break;
            case +Binop::Multiplication:
                binop_pointer_ = &model::INumericType::Mul;
                break;
            case +Binop::Division:
                binop_pointer_ = &model::INumericType::Div;
                break;
            default:
                throw config::ConfigurationError(
                        "Invalid operation for algebraic constraints discovery");
        }
    };
    auto check_weight = [](double parameter) {
        if (parameter <= 0 || parameter > 1)
            throw config::ConfigurationError("weight out of range");
    };
    auto check_p_fuzz = [](double parameter) {
        if (parameter <= 0 || parameter >= 1)
            throw config::ConfigurationError("p_fuzz out of range");
    };
    auto check_fuzziness = [](double parameter) {
        if (parameter < 0 || parameter > 1) config::ConfigurationError("Parameter out of range");
    };
    auto check_non_negative = [](int parameter) {
        if (parameter < 0) throw config::ConfigurationError("Parameter out of range");
    };
    auto check_positive = [](int parameter) {
        if (parameter <= 0) throw config::ConfigurationError("Parameter out of range");
    };

    RegisterOption(config::TableOpt(&input_table_));
    RegisterOption(Option{&bin_operation_, kBinaryOperation, kDBinaryOperation}.SetValueCheck(
            check_and_set_binop));
    RegisterOption(Option{&fuzziness_, kFuzziness, kDFuzziness}.SetValueCheck(check_fuzziness));
    RegisterOption(Option{&p_fuzz_, kFuzzinessProbability, kDFuzzinessProbability}.SetValueCheck(
            check_p_fuzz));
    RegisterOption(Option{&weight_, kWeight, kDWeight}.SetValueCheck(check_weight));
    RegisterOption(
            Option{&bumps_limit_, kBumpsLimit, kDBumpsLimit}.SetValueCheck(check_non_negative));
    RegisterOption(Option{&iterations_limit_, kIterationsLimit, kDIterationsLimit}.SetValueCheck(
            check_positive));
    RegisterOption(Option{&seed_, kACSeed, kDACSeed});
}

void ACAlgorithm::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_,
                                                                       false);  // nulls are ignored
}

void ACAlgorithm::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kFuzziness, kFuzzinessProbability, kWeight, kBumpsLimit, kIterationsLimit,
                          kACSeed, kBinaryOperation});
}

void ACAlgorithm::ResetState() {
    ac_pairs_.clear();
    ranges_.clear();
    ac_exception_finder_->ResetState();
}

size_t ACAlgorithm::CalculateSampleSize(size_t k_bumps) const {
    if (fuzziness_ == 0) {
        return typed_relation_->GetNumRows();
    }

    /* Calculation of formula 26.2.23 from <<Mathematical Tables>>
    by Abramowitz & Stegun. Constants are given */
    constexpr double c0 = 2.515517;
    constexpr double c1 = 0.802853;
    constexpr double c2 = 0.010328;
    constexpr double d1 = 1.432788;
    constexpr double d2 = 0.189269;
    constexpr double d3 = 0.001308;
    assert(p_fuzz_ != 1);
    double t = sqrt(log(1 / pow(1.0 - p_fuzz_, 2.0)));
    double t_2 = pow(t, 2.0);
    double t_3 = pow(t, 3.0);
    double xp = t - ((c0 + c1 * t + c2 * t_2) / (1 + d1 * t + d2 * t_2 + d3 * t_3));
    /* Calculation of formula 26.4.17 from <<Mathematical Tables>> by Abramowitz & Stegun*/
    double freedom_degree = 2 * (k_bumps + 1);
    double tmp1 = 2 / (9 * freedom_degree);
    double tmp2 = (1 - tmp1 + xp * sqrt(tmp1));
    double Xp_2 = freedom_degree * pow(tmp2, 3.0);
    /* Formula (7) from <<BHUNT: Automatic Discovery of Fuzzy Algebraic Constraints
     * in Relational Data>> by Paul G. Brown & Peter J. Haas*/
    size_t sample_size = (Xp_2 * (2 - fuzziness_)) / (4 * fuzziness_) + k_bumps / 2.0;

    return sample_size;
}

std::vector<std::byte const*> ACAlgorithm::Sampling(std::vector<model::TypedColumnData> const& data,
                                                    size_t lhs_i, size_t rhs_i) {
    ACPairs ac_pairs;
    std::vector<std::byte const*> ranges;
    size_t k_bumps = 1;
    size_t i = 0;
    size_t sample_size = CalculateSampleSize(k_bumps);
    size_t new_k_bumps = 1;
    size_t n_rows = data.at(lhs_i).GetData().size();
    double probability = sample_size / static_cast<double>(n_rows);
    while (i < iterations_limit_ &&
           (ranges.empty() || sample_size < CalculateSampleSize(new_k_bumps))) {
        k_bumps = new_k_bumps;
        sample_size = CalculateSampleSize(k_bumps);
        probability = sample_size / static_cast<double>(n_rows);
        ranges = SamplingIteration(data, lhs_i, rhs_i, probability, ac_pairs);
        new_k_bumps = ranges.size() / 2;
        if (new_k_bumps == 0) {
            new_k_bumps = k_bumps + 1;
        }
        ++i;
    }
    RestrictRangesAmount(ranges);
    ac_pairs_.emplace_back(ACPairsCollection(
            model::CreateSpecificType<model::INumericType>(data.at(lhs_i).GetTypeId(), true),
            std::move(ac_pairs), lhs_i, rhs_i));
    return ranges;
}

std::vector<std::byte const*> ACAlgorithm::SamplingIteration(
        std::vector<model::TypedColumnData> const& data, size_t lhs_i, size_t rhs_i,
        double probability, ACPairs& ac_pairs) {
    std::vector<std::byte const*> const& lhs = data.at(lhs_i).GetData();
    std::vector<std::byte const*> const& rhs = data.at(rhs_i).GetData();
    ac_pairs.clear();
    std::mt19937 gen(seed_);

    std::bernoulli_distribution d(probability);
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (d(gen)) {
            std::byte const* l = lhs.at(i);
            std::byte const* r = rhs.at(i);
            if (data[lhs_i].IsNullOrEmpty(i) || data[rhs_i].IsNullOrEmpty(i)) {
                continue;
            }
            auto res = std::unique_ptr<std::byte[]>(num_type_->Allocate());
            num_type_->ValueFromStr(res.get(), "0");
            if (bin_operation_ == +Binop::Division &&
                num_type_->Compare(r, res.get()) == model::CompareResult::kEqual) {
                continue;
            }
            InvokeBinop(l, r, res.get());
            auto ac = std::make_unique<ACPair>(ACPair::ColumnValueIndex{lhs_i, i},
                                               ACPair::ColumnValueIndex{rhs_i, i}, l, r,
                                               std::move(res));
            ac_pairs.emplace_back(std::move(ac));
        }
    }

    std::sort(ac_pairs.begin(), ac_pairs.end(),
              [this](std::unique_ptr<ACPair> const& a, std::unique_ptr<ACPair> const& b) {
                  return model::CompareResult::kLess ==
                         this->num_type_->Compare(a->GetRes(), b->GetRes());
              });

    return ConstructDisjunctiveRanges(ac_pairs);
}

void ACAlgorithm::RestrictRangesAmount(std::vector<std::byte const*>& ranges) const {
    if (bumps_limit_ == 0) {
        return;
    }

    size_t bumps = ranges.size() / 2;

    if (bumps == 1) {
        return;
    }

    while (bumps > bumps_limit_) {
        double min_dist = -1;
        size_t min_index = 1;
        for (size_t i = min_index; i < bumps * 2 - 1; i += 2) {
            double dist = num_type_->Dist(ranges.at(i), ranges.at(i + 1));
            if (min_dist == -1 || dist < min_dist) {
                min_dist = dist;
                min_index = i;
            }
        }
        ranges.erase(ranges.begin() + min_index);
        ranges.erase(ranges.begin() + min_index);
        --bumps;
    }
}

RangesCollection const& ACAlgorithm::GetRangesByColumns(size_t lhs_i, size_t rhs_i) const {
    auto res =
            std::find_if(ranges_.begin(), ranges_.end(), [lhs_i, rhs_i](RangesCollection const& r) {
                return (r.col_pair.col_i.first == lhs_i && r.col_pair.col_i.second == rhs_i);
            });
    if (res == ranges_.end()) {
        throw std::invalid_argument("No ranges for selected pair of columns");
    }
    return *res;
}

ACPairsCollection const& ACAlgorithm::GetACPairsByColumns(size_t lhs_i, size_t rhs_i) const {
    auto res = std::find_if(
            ac_pairs_.begin(), ac_pairs_.end(), [lhs_i, rhs_i](ACPairsCollection const& c) {
                return (c.col_pair.col_i.first == lhs_i && c.col_pair.col_i.second == rhs_i);
            });
    if (res == ac_pairs_.end()) {
        throw std::invalid_argument("No ac_pairs for selected pair of columns");
    }
    return *res;
}
void ACAlgorithm::PrintRanges(std::vector<model::TypedColumnData> const& data) const {
    for (size_t i = 0; i < ranges_.size(); ++i) {
        LOG(DEBUG) << "lhs: " << data.at(ranges_[i].col_pair.col_i.first).ToString() << std::endl;
        LOG(DEBUG) << "rhs: " << data.at(ranges_[i].col_pair.col_i.second).ToString() << std::endl;
        if (ranges_[i].ranges.empty()) {
            LOG(DEBUG) << "No intervals were found." << std::endl;
            continue;
        }
        for (size_t k = 0; k < ranges_[i].ranges.size() - 1; k += 2) {
            auto* num_type = ranges_[i].col_pair.num_type.get();
            LOG(DEBUG) << "[" << num_type->ValueToString(ranges_[i].ranges[k]) << ", "
                       << num_type->ValueToString(ranges_[i].ranges[k + 1]) << "]";
            if (k != ranges_[i].ranges.size() - 2) {
                LOG(DEBUG) << ", ";
            }
        }
        LOG(DEBUG) << std::endl;
    }
}

std::vector<std::byte const*> ACAlgorithm::ConstructDisjunctiveRanges(
        ACPairs const& ac_pairs) const {
    std::vector<std::byte const*> ranges;
    if (ac_pairs.size() < 2) {
        ranges = std::vector<std::byte const*>();
        return ranges;
    }

    ACPair const* l_border = ac_pairs.front().get();
    ACPair const* r_border = nullptr;

    if (weight_ < 1) {
        double delta = num_type_->Dist(ac_pairs.front()->GetRes(), ac_pairs.back()->GetRes()) *
                       (weight_ / (1 - weight_));

        for (size_t i = 0; i < ac_pairs.size() - 1; ++i) {
            if (num_type_->Dist(ac_pairs[i]->GetRes(), ac_pairs[i + 1]->GetRes()) <= delta) {
                r_border = ac_pairs[i + 1].get();
            } else {
                ranges.emplace_back(l_border->GetRes());
                ranges.emplace_back(ac_pairs[i]->GetRes());
                l_border = ac_pairs[i + 1].get();
                r_border = ac_pairs[i + 1].get();
            }
        }
    } else {
        assert(weight_ == 1);
        r_border = ac_pairs.back().get();
    }

    if (r_border == ac_pairs.back().get()) {
        ranges.emplace_back(l_border->GetRes());
        ranges.emplace_back(r_border->GetRes());
    }

    return ranges;
}

RangesCollection ACAlgorithm::ReconstructRangesByColumns(size_t lhs_i, size_t rhs_i,
                                                         double weight) {
    SetOption(config::names::kWeight, weight);
    ACPairsCollection const& constraints_collection = GetACPairsByColumns(lhs_i, rhs_i);
    ACPairs const& ac_pairs = constraints_collection.ac_pairs;
    std::vector<std::byte const*> ranges = ConstructDisjunctiveRanges(ac_pairs);
    model::TypeId type_id = constraints_collection.col_pair.num_type->GetTypeId();
    return RangesCollection{model::CreateSpecificType<model::INumericType>(type_id, true),
                            std::move(ranges), lhs_i, rhs_i};
}

unsigned long long ACAlgorithm::ExecuteInternal() {
    std::vector<model::TypedColumnData> const& data = typed_relation_->GetColumnData();
    if (data.empty()) {
        throw std::runtime_error("Empty table was given.");
    }
    auto start_time = std::chrono::system_clock::now();

    for (size_t col_i = 0; col_i < data.size() - 1; ++col_i) {
        if (!data.at(col_i).GetType().IsNumeric()) continue;
        num_type_ =
                model::CreateSpecificType<model::INumericType>(data.at(col_i).GetTypeId(), true);
        for (size_t col_k = col_i + 1; col_k < data.size(); ++col_k) {
            if (data.at(col_i).GetTypeId() == data.at(col_k).GetTypeId()) {
                ranges_.emplace_back(
                        RangesCollection{model::CreateSpecificType<model::INumericType>(
                                                 data.at(col_i).GetTypeId(), true),
                                         Sampling(data, col_i, col_k), col_i, col_k});
                /* Because of asymmetry and division by 0, we need to rediscover ranges.
                 * We don't need to do that for minus: (column1 - column2) lies in *some ranges*
                 * there we can express one column through another without possible problems */
                if (bin_operation_ == +Binop::Division) {
                    ranges_.emplace_back(
                            RangesCollection{model::CreateSpecificType<model::INumericType>(
                                                     data.at(col_i).GetTypeId(), true),
                                             Sampling(data, col_k, col_i), col_k, col_i});
                }
            }
        }
    }

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    PrintRanges(data);
    return elapsed_milliseconds.count();
}

}  // namespace algos
