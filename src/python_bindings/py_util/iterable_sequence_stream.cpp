#include "iterable_sequence_stream.h"

#include <string>

#include "core/config/exceptions.h"

namespace python_bindings {

namespace py = pybind11;

IterableSequenceStream::IterableSequenceStream(py::iterable sequence)
    : sequence_(std::move(sequence)) {
    // Do not call Initialize() here to avoid consuming the first element of a generator
    index_ = 0;
    expect_explicit_timestamp_ = std::nullopt;
    initialized_ = false;
}

void IterableSequenceStream::Initialize() {
    iterator_ = sequence_.begin();
    index_ = 0;
    expect_explicit_timestamp_ = std::nullopt;
    initialized_ = true;
}

bool IterableSequenceStream::HasNext() {
    if (!initialized_) {
        Initialize();
    }
    return iterator_ != py::iterator::sentinel();
}

model::TimedEventSet IterableSequenceStream::GetNext() {
    if (!initialized_) {
        Initialize();
    }

    // Capture the item as a strong reference because advancing the iterator
    // might release the reference held by the iterator.
    py::object item = py::reinterpret_borrow<py::object>(*iterator_);
    ++iterator_;

    bool is_tuple = py::isinstance<py::tuple>(item);

    if (!expect_explicit_timestamp_.has_value()) {
        expect_explicit_timestamp_ = is_tuple;
    } else if (*expect_explicit_timestamp_ != is_tuple) {
        throw config::ConfigurationError(
                "Inconsistent sequence data: mixed explicit (tuple) and implicit (list/iterable) "
                "timestamps.");
    }

    py::handle events_obj;
    model::Timestamp timestamp = static_cast<model::Timestamp>(index_);

    if (is_tuple) {
        auto tuple = py::cast<py::tuple>(item);
        if (py::len(tuple) != 2) {
            throw config::ConfigurationError(
                    "Tuple in sequence data must have 2 elements: (events, timestamp).");
        }
        events_obj = tuple[0];
        timestamp = py::cast<model::Timestamp>(tuple[1]);
    } else {
        events_obj = item;
    }

    if (!py::isinstance<py::iterable>(events_obj)) {
        throw config::ConfigurationError("Events entry must be an iterable of events.");
    }

    std::vector<model::Event> events;
    for (auto event : events_obj) {
        try {
            events.push_back(py::cast<model::Event>(event));
        } catch (py::cast_error const&) {
            throw config::ConfigurationError(
                    "Invalid event type in python sequence: expected unsigned int (64-bit).");
        }
    }

    index_++;
    model::TimedEventSet record{std::move(events), std::move(timestamp)};
    if (!record.IsSortedUnique()) {
        throw config::ConfigurationError(
                "Events in a transaction are not sorted or contain duplicates.");
    }
    return record;
}

}  // namespace python_bindings
