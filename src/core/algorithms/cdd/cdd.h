#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"

namespace model {
enum class ConditionOp {
    EQ,           // =
    NEQ,          // !=
    LT,           // <
    LE,           // <=
    GT,           // >
    GE,           // >=
    IN_SET,       // ∈ {a, b, c}
    IN_INTERVAL,  // ∈ [x, y]
    ANY           // _
};

using ConditionValue = std::variant<std::monostate, int64_t, double, std::string>;

using ConditionLimit = std::variant<ConditionValue, std::vector<ConditionValue>,
                                    std::pair<ConditionValue, ConditionValue>>;

struct Condition {
    std::string attribute_;
    ConditionLimit value_;
    ConditionOp op_;

    mutable std::optional<std::vector<std::byte>> cached_bin_{};
    mutable std::optional<std::vector<std::vector<std::byte>>> cached_set_bins_{};
    mutable std::optional<std::pair<std::vector<std::byte>, std::vector<std::byte>>>
            cached_interval_bins_{};

    bool Match(model::IMetrizableType const& type, std::byte const* data) const {
        if (op_ == ConditionOp::ANY) return true;

        return std::visit(
                [&](auto&& arg) -> bool {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, ConditionValue>) {
                        model::CompareResult res = CompareWithValue(type, data, arg);
                        return CheckOp(res, op_);
                    } else if constexpr (std::is_same_v<T, std::vector<ConditionValue>>) {
                        if (op_ != ConditionOp::IN_SET) return false;
                        return std::any_of(arg.begin(), arg.end(), [&](ConditionValue const& v) {
                            return CompareWithValue(type, data, v) == model::CompareResult::kEqual;
                        });
                    } else if constexpr (std::is_same_v<
                                                 T, std::pair<ConditionValue, ConditionValue>>) {
                        if (op_ != ConditionOp::IN_INTERVAL) return false;

                        model::CompareResult cmp_min = CompareWithValue(type, data, arg.first);
                        if (cmp_min == model::CompareResult::kLess) return false;
                        model::CompareResult cmp_max = CompareWithValue(type, data, arg.second);
                        if (cmp_max == model::CompareResult::kGreater) return false;
                        return true;
                    }
                    return false;
                },
                value_);
    }

    [[nodiscard]] std::string ToString() const {
        std::ostringstream oss;
        oss << attribute_ << " ";

        switch (op_) {
            case ConditionOp::EQ:
                oss << "=";
                break;
            case ConditionOp::NEQ:
                oss << "!=";
                break;
            case ConditionOp::LT:
                oss << "<";
                break;
            case ConditionOp::LE:
                oss << "<=";
                break;
            case ConditionOp::GT:
                oss << ">";
                break;
            case ConditionOp::GE:
                oss << ">=";
                break;
            case ConditionOp::IN_SET:
                oss << "∈";
                break;
            case ConditionOp::IN_INTERVAL:
                oss << "∈";
                break;
            case ConditionOp::ANY:
                oss << "_";
                break;
        }
        oss << " ";

        std::visit(
                [&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, ConditionValue>) {
                        oss << ValToStr(arg);
                    } else if constexpr (std::is_same_v<T, std::vector<ConditionValue>>) {
                        oss << "{";
                        for (size_t i = 0; i < arg.size(); ++i) {
                            oss << ValToStr(arg[i]);
                            if (i < arg.size() - 1) oss << ", ";
                        }
                        oss << "}";
                    } else if constexpr (std::is_same_v<
                                                 T, std::pair<ConditionValue, ConditionValue>>) {
                        oss << "[" << ValToStr(arg.first) << ", " << ValToStr(arg.second) << "]";
                    }
                },
                value_);

        return oss.str();
    }

    bool operator==(Condition const& other) const {
        if (attribute_ != other.attribute_ || op_ != other.op_) return false;
        return CompareConditionLimit(value_, other.value_);
    }

private:
    static bool CheckOp(model::CompareResult res, ConditionOp op) {
        switch (op) {
            case ConditionOp::EQ:
                return res == model::CompareResult::kEqual;
            case ConditionOp::NEQ:
                return res != model::CompareResult::kEqual;
            case ConditionOp::GT:
                return res == model::CompareResult::kGreater;
            case ConditionOp::GE:
                return res != model::CompareResult::kLess;
            case ConditionOp::LT:
                return res == model::CompareResult::kLess;
            case ConditionOp::LE:
                return res != model::CompareResult::kGreater;
            default:
                return false;
        }
    }

    static bool IsEqual(double a, double b) {
        constexpr double kRelativeTolerance = 1e-9;
        if (std::isnan(a) && std::isnan(b)) return true;
        if (std::isinf(a) && std::isinf(b)) return a == b;
        return std::abs(a - b) <= kRelativeTolerance * std::max(std::abs(a), std::abs(b));
    }

    static bool CompareConditionValue(ConditionValue const& a, ConditionValue const& b) {
        if (a.index() != b.index()) return false;
        return std::visit(
                [&](auto&& arg_a) -> bool {
                    using T = std::decay_t<decltype(arg_a)>;
                    if constexpr (std::is_same_v<T, std::monostate>)
                        return true;
                    else if constexpr (std::is_same_v<T, std::string>)
                        return arg_a == std::get<std::string>(b);
                    else if constexpr (std::is_same_v<T, double>)
                        return IsEqual(arg_a, std::get<double>(b));
                    else
                        return arg_a == std::get<T>(b);
                },
                a);
    }

    static bool CompareConditionLimit(ConditionLimit const& a, ConditionLimit const& b) {
        if (a.index() != b.index()) return false;
        return std::visit(
                [&](auto&& arg_a) -> bool {
                    using T = std::decay_t<decltype(arg_a)>;
                    if constexpr (std::is_same_v<T, ConditionValue>) {
                        return CompareConditionValue(arg_a, std::get<ConditionValue>(b));
                    } else if constexpr (std::is_same_v<T, std::vector<ConditionValue>>) {
                        auto const& vec_b = std::get<std::vector<ConditionValue>>(b);
                        if (arg_a.size() != vec_b.size()) return false;
                        for (size_t i = 0; i < arg_a.size(); ++i) {
                            if (!CompareConditionValue(arg_a[i], vec_b[i])) return false;
                        }
                        return true;
                    } else if constexpr (std::is_same_v<
                                                 T, std::pair<ConditionValue, ConditionValue>>) {
                        auto const& pr_b = std::get<std::pair<ConditionValue, ConditionValue>>(b);
                        return CompareConditionValue(arg_a.first, pr_b.first) &&
                               CompareConditionValue(arg_a.second, pr_b.second);
                    }
                    return false;
                },
                a);
    }

    static std::vector<std::byte> ValueToBin(ConditionValue const& v) {
        return std::visit(
                [](auto&& arg) -> std::vector<std::byte> {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::monostate>)
                        return std::vector<std::byte>{};
                    else {
                        std::vector<std::byte> buf(sizeof(T));
                        std::memcpy(buf.data(), &arg, sizeof(T));
                        return buf;
                    }
                },
                v);
    }

    static std::string ValToStr(ConditionValue const& v) {
        return std::visit(
                [](auto&& arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::monostate>)
                        return std::string("");
                    else if constexpr (std::is_same_v<T, std::string>)
                        return arg;
                    else
                        return std::to_string(arg);
                },
                v);
    }

    model::CompareResult CompareWithValue(model::IMetrizableType const& type, std::byte const* data,
                                          ConditionValue const& v) const {
        if (std::holds_alternative<std::string>(v)) {
            return type.Compare(data,
                                reinterpret_cast<std::byte const*>(&std::get<std::string>(v)));
        }
        if (!cached_bin_.has_value()) {
            cached_bin_ = ValueToBin(v);
        }
        return type.Compare(data, cached_bin_->data());
    }
};

struct CDD {
    model::DDString dd_;
    std::vector<Condition> lhs_condition_;
    std::vector<Condition> rhs_condition_;

    static std::optional<std::size_t> IsCondsHolds(
            std::vector<Condition> const& conds,
            std::unique_ptr<model::ColumnLayoutTypedRelationData> const& typed_relation,
            std::size_t id) {
        for (std::size_t i = 0; i < conds.size(); ++i) {
            auto const& cond = conds[i];
            auto column_index = typed_relation->GetSchema()->GetColumn(cond.attribute_)->GetIndex();
            model::TypedColumnData const& column = typed_relation->GetColumnData(column_index);
            auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
            if (!cond.Match(type, column.GetValue(id))) {
                return i;
            }
        }
        return std::nullopt;
    }

    std::string ToString() const {
        auto conds_to_string = [](std::vector<Condition> const& conds) {
            std::string res;
            for (size_t i = 0; i < conds.size(); ++i) {
                res += conds[i].ToString();
                if (i < conds.size() - 1) res += ", ";
            }
            return res;
        };

        std::ostringstream oss;
        oss << dd_.ToString() << ", LHS conditions: (" << conds_to_string(lhs_condition_)
            << "), RHS conditions: (" << conds_to_string(rhs_condition_) << ")";
        return oss.str();
    }

    bool operator==(CDD const& other) const = default;
};
}  // namespace model
