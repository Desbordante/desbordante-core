#include "ACAlgorithm.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <random>

#include "types/CreateType.h"

namespace algos {

size_t ACAlgorithm::CalculateSampleSize(size_t k_bumps) const {
    double xp;
    double c0, c1, c2, d1, d2, d3, t, t_2, t_3;
    c0 = 2.515517;
    c1 = 0.802853;
    c2 = 0.010328;
    d1 = 1.432788;
    d2 = 0.189269;
    d3 = 0.001308;
    t = sqrt(log(1 / pow(1.0 - p_fuzz_, 2.0)));
    t_2 = pow(t, 2.0);
    t_3 = pow(t, 3.0);

    xp = t - ((c0 + c1 * t + c2 * t_2) / (1 + d1 * t + d2 * t_2 + d3 * t_3));

    double freedom_degree = 2 * (k_bumps + 1);
    double tmp1 = 2 / (9 * freedom_degree);
    double tmp2 = (1 - tmp1 + xp * sqrt(tmp1));
    double Xp_2 = freedom_degree * pow(tmp2, 3.0);

    size_t sample_size = (Xp_2 * (2 - fuzziness_)) / (4 * fuzziness_) + k_bumps / 2.0;
    return sample_size;
}

std::vector<std::byte*> ACAlgorithm::Sampling(std::vector<model::TypedColumnData> const& data,
                                              size_t lhs_i, size_t rhs_i, size_t k_bumps = 1,
                                              size_t i = 0) {
    size_t sample_size = CalculateSampleSize(k_bumps);
    const std::vector<const std::byte*>& lhs = data.at(lhs_i).GetData();
    const std::vector<const std::byte*>& rhs = data.at(rhs_i).GetData();
    Constraints constraints;
    size_t n_rows = lhs.size();
    double probability = sample_size / static_cast<double>(n_rows);
    std::random_device rd;
    std::mt19937 gen(rd());

    if (test_mode_) {
        gen = std::mt19937(0);
    }

    std::bernoulli_distribution d(probability);
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (d(gen)) {
            const std::byte* l = lhs.at(i);
            const std::byte* r = rhs.at(i);
            if (l == nullptr || r == nullptr) {
                continue;
            }
            auto* res = new std::byte[num_type_->GetSize()];
            num_type_->ValueFromStr(res, "0");
            if (bin_operation_ == '/' &&
                num_type_->Compare(r, res) == model::CompareResult::kEqual) {
                delete[] res;
                continue;
            }
            switch (bin_operation_) {
            case '+':
                num_type_->Add(l, r, res);
                break;
            case '-':
                num_type_->Sub(l, r, res);
                break;
            case '*':
                num_type_->Mul(l, r, res);
                break;
            case '/':
                num_type_->Div(l, r, res);
            default:
                break;
            }
            std::unique_ptr<model::AC> ac =
                std::make_unique<model::AC>(std::pair<size_t, size_t>(lhs_i, i),
                                            std::pair<size_t, size_t>(rhs_i, i), l, r, res);
            constraints.emplace_back(std::move(ac));
        }
    }

    auto& n = num_type_;
    std::sort(constraints.begin(), constraints.end(),
              [&n](const std::unique_ptr<model::AC>& a, const std::unique_ptr<model::AC>& b) {
                  return model::CompareResult::kLess == n->Compare(a->res, b->res);
              });

    std::vector<std::byte*> ranges;
    ConstructDisjunctiveRanges(constraints, ranges);
    if (i >= iterations_limit) {
        RestrictRangesAmount(ranges);
        return ranges;
    }
    size_t new_k_bumps = ranges.size() / 2;
    if (sample_size < CalculateSampleSize(new_k_bumps)) {
        return Sampling(data, lhs_i, rhs_i, new_k_bumps, ++i);
    }
    if (ranges.empty()) {
        return Sampling(data, lhs_i, rhs_i, k_bumps + 1, ++i);
    }
    RestrictRangesAmount(ranges);
    alg_constraints_.emplace_back(std::move(constraints));

    return ranges;
}

void ACAlgorithm::RestrictRangesAmount(std::vector<std::byte*>& ranges) {
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
        ranges.erase(ranges.begin() + min_index + 1);
        --bumps;
    }
}

void ACAlgorithm::PrintRanges(std::vector<model::TypedColumnData> const& data) const {
    for (size_t i = 0; i < ranges_.size(); ++i) {
        std::cout << "lhs: " << data.at(ranges_[i].column_indices.first).ToString() << std::endl;
        std::cout << "rhs: " << data.at(ranges_[i].column_indices.second).ToString() << std::endl;
        if (ranges_[i].ranges.empty()) {
            std::cout << "No intervals were found." << std::endl;
            continue;
        }
        for (size_t k = 0; k < ranges_[i].ranges.size() - 1; k += 2) {
            auto* num_type = dynamic_cast<model::INumericType*>(ranges_[i].num_type.get());
            std::cout << "[" << num_type->ValueToString(ranges_[i].ranges[k]) << ", "
                      << num_type->ValueToString(ranges_[i].ranges[k + 1]) << "]";
            if (k != ranges_[i].ranges.size() - 2) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
}

void ACAlgorithm::ConstructDisjunctiveRanges(Constraints& constraints,
                                             std::vector<std::byte*>& ranges) {
    if (constraints.empty()) {
        ranges = std::vector<std::byte*>();
        return;
    }
    model::AC* l_border = (constraints.begin())->get();
    model::AC* r_border = nullptr;
    double delta = num_type_->Dist((*constraints.begin())->res, (*(constraints.end() - 1))->res) *
                   (weight_ / (1 - weight_));

    for (size_t i = 0; i < constraints.size() - 1; ++i) {
        if (num_type_->Dist(constraints[i]->res, constraints[i + 1]->res) <= delta) {
            r_border = (constraints[i + 1]).get();
        } else {
            ranges.emplace_back(l_border->res);
            ranges.emplace_back(constraints[i]->res);
            l_border = (constraints[i + 1]).get();
            r_border = (constraints[i + 1]).get();
        }
    }
    if (r_border == (constraints.end() - 1)->get()) {
        ranges.emplace_back(l_border->res);
        ranges.emplace_back(r_border->res);
    }
}

unsigned long long ACAlgorithm::Execute() {
    std::vector<model::TypedColumnData> const& data = typed_relation_->GetColumnData();
    if (data.empty()) {
        throw std::runtime_error("Empty table was given.");
    }

    auto start_time = std::chrono::system_clock::now();

    for (size_t col_i = 0; col_i < data.size() - 1; ++col_i) {
        if (!data.at(col_i).GetType().IsNumeric()) continue;

        for (size_t col_k = col_i + 1; col_k < data.size(); ++col_k) {
            if (data.at(col_i).GetTypeId() == data.at(col_k).GetTypeId()) {
                auto numeric =
                    model::CreateType(data.at(col_i).GetTypeId(), true);  // Is null == null??
                num_type_ = dynamic_cast<model::INumericType*>(numeric.get());

                ranges_.emplace_back(RangesCollection{std::move(numeric),
                                                      Sampling(data, col_i, col_k), col_i, col_k});

                if (bin_operation_ == '-' || bin_operation_ == '/') {
                    ranges_.emplace_back(RangesCollection{
                        std::move(model::CreateType(data.at(col_i).GetTypeId(), true)),
                        Sampling(data, col_k, col_i), col_k, col_i});
                }
            }
        }
    }

    PrintRanges(data);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

}  // namespace algos
