#pragma once

#include "algorithms/algorithm.h"

namespace util::config::names {
extern const char* const kData;
}

namespace util::config::descriptions {
extern const char* const kDData;
}

namespace util::config {
template <typename T>
class Option;
}

namespace algos {

template <typename DataType>
class AlgorithmWithData : public Algorithm {
private:
    void RegisterOptions() {
        RegisterOption(util::config::Option{&data_, util::config::names::kData,
                                            util::config::descriptions::kDData});
    }

protected:
    DataType data_;

    explicit AlgorithmWithData(std::vector<std::string_view> phase_names)
        : Algorithm(std::move(phase_names)) {
        RegisterOptions();
        MakeOptionsAvailable({util::config::names::kData});
    }
};

}  // namespace algos
