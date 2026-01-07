#pragma once

#pragma once

#include <cstddef>

#include "core/model/sequence/timed_event_set.h"

namespace model {

class ISequenceStream {
public:
    virtual ~ISequenceStream() = default;

    /**
     * @brief Checks if there are more elements in the sequence.
     * @return true if the stream has more elements, false otherwise.
     */
    virtual bool HasNext() = 0;

    /**
     * @brief Retrieves the next set of events from the stream.
     *
     * Contract guarantees:
     * - The events within the returned TimedEventSet are sorted.
     * - The events within the returned TimedEventSet are unique.
     *
     * @note The stream does NOT guarantee that the sets themselves are sorted by timestamp
     *       or that timestamps are unique across the stream. The consumer must handle global
     *       ordering and uniqueness if required.
     * @return A TimedEventSet containing the next batch of events and their timestamp.
     */
    virtual TimedEventSet GetNext() = 0;
};

}  // namespace model
