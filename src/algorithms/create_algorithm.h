#pragma once

#include <boost/mp11.hpp>

#include "algorithms/algorithm_types.h"

namespace algos {

using AlgorithmTypes =
        std::tuple<Depminer, DFD, FastFDs, FDep, Fd_mine, Pyro, Tane, FUN, hyfd::HyFD, Aid, Apriori,
                   metric::MetricVerifier, DataStats, fd_verifier::FDVerifier, HyUCC,
                   cfd::FDFirstAlgorithm, ACAlgorithm>;

template <typename AlgorithmBase = Algorithm>
std::unique_ptr<AlgorithmBase> CreateAlgorithmInstance(AlgorithmType algorithm) {
    auto const create = [](auto I) -> std::unique_ptr<AlgorithmBase> {
        using AlgorithmType = std::tuple_element_t<I, AlgorithmTypes>;
        if constexpr (std::is_convertible_v<AlgorithmType *, AlgorithmBase *>) {
            return std::make_unique<AlgorithmType>();
        } else {
            throw std::invalid_argument(
                    "Cannot use " + boost::typeindex::type_id<AlgorithmType>().pretty_name() +
                    " as " + boost::typeindex::type_id<AlgorithmBase>().pretty_name());
        }
    };

    return boost::mp11::mp_with_index<std::tuple_size<AlgorithmTypes>>(
            static_cast<size_t>(algorithm), create);
}

template <typename AlgorithmBase>
std::vector<AlgorithmType> GetAllDerived() {
    auto const is_derived = [](auto I) -> bool {
        using AlgorithmType = std::tuple_element_t<I, AlgorithmTypes>;
        return std::is_base_of_v<AlgorithmBase, AlgorithmType>;
    };
    std::vector<AlgorithmType> derived_from_base{};
    for (AlgorithmType algo : AlgorithmType::_values()) {
        if (boost::mp11::mp_with_index<std::tuple_size<AlgorithmTypes>>(static_cast<size_t>(algo),
                                                                        is_derived)) {
            derived_from_base.push_back(algo);
        }
    }
    return derived_from_base;
}

}  // namespace algos
