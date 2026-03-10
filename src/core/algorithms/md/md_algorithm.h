#pragma once

#include "core/algorithms/algorithm.h"
#include "core/algorithms/md/md.h"
#include "core/util/primitive_collection.h"

namespace algos {

class MdAlgorithm : public Algorithm {
private:
    util::PrimitiveCollection<model::MD> md_collection_;

    void ResetState() override {
        md_collection_.Clear();
        ResetStateMd();
    }

    virtual void ResetStateMd() = 0;

protected:
    void RegisterMd(model::MD md_to_register) {
        md_collection_.Register(std::move(md_to_register));
    }

public:
    std::list<model::MD> const& MdList() const noexcept {
        return md_collection_.AsList();
    }
};

}  // namespace algos
