#include "fem_algorithm.h"

#include <optional>
#include <vector>

#include "core/config/names.h"
#include "core/config/option.h"

namespace algos {

void FEMAlgorithm::LoadDataInternal() {
    auto stream = sequence_stream_;
    if (!stream) throw std::runtime_error("Sequence data stream is null.");

    std::vector<model::TimedEventSet> timed_event_sets;
    std::optional<model::Timestamp> last_timestamp;

    while (stream->HasNext()) {
        auto record = stream->GetNext();

        if (record.begin() == record.end()) continue;

        auto current_timestamp = record.GetTimestamp();
        if (last_timestamp.has_value() && current_timestamp <= *last_timestamp) {
            throw std::runtime_error(
                    "Sequence data is not sorted by timestamp or contains duplicates.");
        }
        last_timestamp = current_timestamp;

        timed_event_sets.push_back(std::move(record));
    }

    event_sequence_ = std::make_unique<model::ComplexEventSequence>(std::move(timed_event_sets));
}

FEMAlgorithm::FEMAlgorithm() : Algorithm({}) {
    RegisterOption(config::Option<std::shared_ptr<model::ISequenceStream>>{
            &sequence_stream_, config::names::kSequence, "Input sequence data"});
    MakeOptionsAvailable({config::names::kSequence});
}

}  // namespace algos
