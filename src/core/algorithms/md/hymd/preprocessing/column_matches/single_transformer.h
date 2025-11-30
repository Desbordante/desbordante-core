#pragma once

#include <functional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/algorithms/md/hymd/indexes/global_value_identifier.h"
#include "core/algorithms/md/hymd/preprocessing/column_matches/transformed_columns_holder.h"
#include "core/model/types/builtin.h"
#include "core/util/desbordante_assume.h"

namespace algos::hymd::preprocessing::column_matches {
template <typename ReturnType>
using TransformFunc = std::function<ReturnType(std::string const&)>;
template <typename L, typename R>
using TransformFuncPair = std::pair<TransformFunc<L>, TransformFunc<R>>;
template <typename L, typename R>
using TransformFuncOption = std::variant<TransformFunc<L>, TransformFuncPair<L, R>>;

namespace detail {
template <typename LDataType, typename RDataType>
struct SimFuncTypes {
public:
    using LeftValueType = LDataType;
    using RightValueType = RDataType;
    using TransformFunctionPair = TransformFuncPair<LeftValueType, RightValueType>;
    using TransformFunctionsOption = TransformFunctionPair;
};

template <typename DataType>
struct SimFuncTypes<DataType, DataType> {
    using LeftValueType = DataType;
    using RightValueType = LeftValueType;
    using TransformFunctionPair = TransformFuncPair<LeftValueType, RightValueType>;
    using TransformFunctionsOption = TransformFuncOption<LeftValueType, RightValueType>;
};

template <typename Type>
struct TypeConverterCallable {
    Type operator()(model::String const& v) const {
        return model::TypeConverter<Type>::kConvert(v);
    }
};

template <>
struct TypeConverterCallable<model::String> {
    model::String operator()(model::String const& v) const {
        return v;
    }
};
}  // namespace detail

template <typename DefaultLeft, typename DefaultRight>
class SingleTransformer {
    using SimFuncStore =
            detail::SimFuncTypes<std::invoke_result_t<DefaultLeft, model::String const&>,
                                 std::invoke_result_t<DefaultRight, model::String const&>>;

public:
    using TransformFunctionsOption = SimFuncStore::TransformFunctionsOption;

private:
    TransformFunctionsOption funcs_;
    DefaultLeft default_left_;
    DefaultRight default_right_;
    using TransformFunctionPair = SimFuncStore::TransformFunctionPair;
    using LeftValueType = SimFuncStore::LeftValueType;
    using RightValueType = SimFuncStore::RightValueType;
    using TransformedColumnsHolderCur = TransformedColumnsHolder<LeftValueType, RightValueType>;
    using SingleFunc = TransformFunc<LeftValueType>;
    using LeftVec = std::vector<LeftValueType>;
    using RightVec = std::vector<RightValueType>;

    auto CreateVec(std::vector<std::string> const& string_values,
                   std::vector<indexes::GlobalValueIdentifier> const& pli_keys, auto&& func,
                   auto&& default_func) const {
        using T = std::invoke_result_t<std::remove_cvref_t<decltype(default_func)>,
                                       model::String const&>;
        std::vector<T> values;
        values.reserve(pli_keys.size());
        if (func) {
            std::for_each(pli_keys.begin(), pli_keys.end(),
                          [&](indexes::GlobalValueIdentifier value_id) {
                              values.push_back(func(string_values[value_id]));
                          });
        } else {
            std::for_each(pli_keys.begin(), pli_keys.end(),
                          [&](indexes::GlobalValueIdentifier value_id) {
                              values.push_back(default_func(string_values[value_id]));
                          });
        }
        return values;
    }

    TransformedColumnsHolderCur CreateSingle(
            std::vector<std::string> const& string_values,
            std::vector<indexes::GlobalValueIdentifier> const& pli_keys, auto&& func) const {
        return {CreateVec(string_values, pli_keys, func, default_left_)};
    }

    TransformedColumnsHolderCur CreatePair(
            std::vector<std::string> const& string_values,
            std::vector<indexes::GlobalValueIdentifier> const& left_pli_keys, auto&& l_func,
            std::vector<indexes::GlobalValueIdentifier> const& right_pli_keys,
            auto&& r_func) const {
        return {CreateVec(string_values, left_pli_keys, l_func, default_left_),
                CreateVec(string_values, right_pli_keys, r_func, default_right_)};
    }

public:
    SingleTransformer(TransformFunctionsOption funcs = {}) : funcs_(std::move(funcs)) {}

    TransformedColumnsHolderCur Transform(
            std::vector<std::string> const& string_values,
            std::vector<indexes::GlobalValueIdentifier> const& left_pli_keys,
            std::vector<indexes::GlobalValueIdentifier> const& right_pli_keys) const {
        if constexpr (std::is_same_v<TransformFunctionsOption, TransformFunctionPair>) {
            auto&& [l_func, r_func] = funcs_;
            return CreatePair(string_values, left_pli_keys, l_func, right_pli_keys, r_func);
        } else {
            bool const potentially_single_column = &left_pli_keys == &right_pli_keys;
            constexpr bool same_defaults = std::is_same_v<DefaultLeft, DefaultRight>;
            if (auto* pair_ptr = std::get_if<TransformFunctionPair>(&funcs_)) {
                auto&& [l_func, r_func] = *pair_ptr;
                if (potentially_single_column && same_defaults && !l_func && !r_func)
                    return CreateSingle(string_values, left_pli_keys, l_func);
                return CreatePair(string_values, left_pli_keys, l_func, right_pli_keys, r_func);
            } else {
                DESBORDANTE_ASSUME(std::holds_alternative<SingleFunc>(funcs_));
                auto&& func = std::get<SingleFunc>(funcs_);
                if (potentially_single_column && (func || same_defaults)) {
                    return CreateSingle(string_values, left_pli_keys, func);
                } else {
                    return CreatePair(string_values, left_pli_keys, func, right_pli_keys, func);
                }
            }
        }
    }
};

template <typename LType, typename RType = LType>
using TypeTransformer = SingleTransformer<detail::TypeConverterCallable<LType>,
                                          detail::TypeConverterCallable<RType>>;
}  // namespace algos::hymd::preprocessing::column_matches
