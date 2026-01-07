#pragma once

#include <optional>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core/model/sequence/isequence_stream.h"

namespace python_bindings {

class IterableSequenceStream : public model::ISequenceStream {
public:
    explicit IterableSequenceStream(pybind11::iterable sequence);

    bool HasNext() override;
    model::TimedEventSet GetNext() override;

private:
    void Initialize();
    pybind11::iterable sequence_;
    pybind11::iterator iterator_;
    size_t index_ = 0;
    std::optional<bool> expect_explicit_timestamp_;
    bool initialized_ = false;
};

}  // namespace python_bindings
