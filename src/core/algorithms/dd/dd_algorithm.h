#pragma once

#include <list>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dd/dd.h"
#include "core/util/primitive_collection.h"

namespace algos::dd {

class DDAlgorithm : public Algorithm {
private:
    util::PrimitiveCollection<model::DDString> dd_collection_;

    void ResetState() override {
        dd_collection_.Clear();
        ResetStateDD();
    }

    virtual void ResetStateDD() = 0;

protected:
    void RegisterDD(model::DDString dd_to_register) {
        dd_collection_.Register(std::move(dd_to_register));
    }

public:
    std::list<model::DDString> const& DDList() const noexcept {
        return dd_collection_.AsList();
    }
};

}  // namespace algos::dd
