#pragma once

#include <filesystem>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"

namespace algos {

class GddValidator : public Algorithm {
private:
    std::filesystem::path graph_path_;
    model::gdd::graph_t graph_;
    std::vector<model::Gdd> gdds_;
    std::vector<model::Gdd> result_;

    void FilterValidGdds();
    void RegisterOptions();

    virtual unsigned long long ExecuteInternal() final;

    virtual void ResetState() final {}
    virtual void LoadDataInternal() final;

protected:
    model::gdd::graph_t const& GetGraph() const noexcept {
        return graph_;
    }

    std::vector<model::Gdd> const& GetGdds() const noexcept {
        return gdds_;
    }

    virtual bool Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph) = 0;

public:
    GddValidator();

    GddValidator(model::gdd::graph_t const& graph, std::vector<model::Gdd> gdds);

    std::vector<model::Gdd> const& GetResult() const noexcept {
        return result_;
    }

    GddValidator(GddValidator const&) = delete;
    GddValidator(GddValidator&&) = delete;
    GddValidator& operator=(GddValidator const&) = delete;
    GddValidator& operator=(GddValidator&&) = delete;
    virtual ~GddValidator() override = default;
};

}  // namespace algos
