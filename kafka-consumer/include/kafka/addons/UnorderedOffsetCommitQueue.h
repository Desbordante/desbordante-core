#pragma once

#include "kafka/Project.h"

#include "kafka/KafkaClient.h"
#include "kafka/Types.h"

#include <deque>
#include <vector>

namespace KAFKA_API {

template <typename T>
class Heap
{
public:
    bool empty() const { return data.empty(); }
    std::size_t size() const { return data.size(); }

    const T& front() const { return data[0]; }

    void push(const T& t)
    {
        data.emplace_back(t);

        for (std::size_t indexCurrent = data.size() - 1; indexCurrent > 0;)
        {
            std::size_t indexParent = (indexCurrent + 1) / 2 - 1;

            if (!(data[indexCurrent] < data[indexParent])) return;

            std::swap(data[indexCurrent], data[indexParent]);
            indexCurrent = indexParent;
        }
    }

    void pop_front()
    {
        data[0] = data.back();
        data.pop_back();

        if (data.empty()) return;

        for (std::size_t indexCurrent = 0;;)
        {
            std::size_t indexRightChild = (indexCurrent + 1) * 2;
            std::size_t indexLeftChild  = indexRightChild - 1;

            if (indexLeftChild >= data.size()) return;

            std::size_t indexMinChild = (indexRightChild >= data.size() || data[indexLeftChild] < data[indexRightChild]) ? indexLeftChild : indexRightChild;

            if (!(data[indexMinChild] < data[indexCurrent])) return;

            std::swap(data[indexCurrent], data[indexMinChild]);
            indexCurrent = indexMinChild;
        }
    }

private:
    std::vector<T> data;
};


/**
 * \brief The queue can be used to determine the right offset to commit.
 *  A `KafkaManuallyCommitConsumer` might forward the received records to different handlers, while these handlers could not ack the records in order.
 *  Then, the `UnorderedOffsetCommitQueue` would help,
 *    1. Prepare an `UnorderedOffsetCommitQueue` for each topic-partition.
 *    2. Make sure call `waitOffset()` for each record received.
 *    3. Make sure call `ackOffset()` while a handler acks for an record.
 *    4. Figure out whether there's offset to commit with `popOffsetToCommit()` and commit the offset then.
 */
class UnorderedOffsetCommitQueue
{
public:
    UnorderedOffsetCommitQueue(const Topic& topic, Partition partition)
        : _partitionInfo(std::string("topic[").append(topic).append("], paritition[").append(std::to_string(partition)).append("]"))
    {
    }
    UnorderedOffsetCommitQueue() = default;

    /**
     * \brief Return how many received offsets have not been popped to commit (with `popOffsetToCommit()`).
     */
    std::size_t size() const { return _offsetsReceived.size(); }

    /**
     * \brief Add an offset (for a ConsumerRecord) to the waiting list, until it being acked (with `ackOffset`).
     * Note: Make sure the offset would be `ack` later with `ackOffset()`.
     */
    void waitOffset(Offset offset)
    {
        if (offset < 0 || (!_offsetsReceived.empty() && offset <= _offsetsReceived.back()))
        {
            // Invalid offset (might be fetched from the record which had no valid offset)
            KAFKA_API_LOG(Log::Level::Err, "Got invalid offset to wait[%lld]! %s", offset, (_partitionInfo.empty() ? "" : _partitionInfo.c_str()));
            return;
        }

        _offsetsReceived.emplace_back(offset);
    }

    /**
     * \brief Ack the record has been handled and ready to be committed.
     * Note: If all offsets ahead has been acked, then with `popOffsetToCommit()`, we'd get `offset + 1`, which is ready to be committed for the consumer.
     */
    void ackOffset(Offset offset)
    {
        Offset maxOffsetReceived = _offsetsReceived.back();
        if (offset > maxOffsetReceived)
        {
            // Runtime error
            KAFKA_API_LOG(Log::Level::Err, "Got invalid ack offset[%lld]! Even larger than all offsets received[%lld]! %s", offset, maxOffsetReceived, (_partitionInfo.empty() ? "" : _partitionInfo.c_str()));
        }

        _offsetsToCommit.push(offset);
        do
        {
            Offset minOffsetToCommit = _offsetsToCommit.front();
            Offset expectedOffset    = _offsetsReceived.front();
            if (minOffsetToCommit == expectedOffset)
            {
                _toCommit = expectedOffset + 1;
                _offsetsToCommit.pop_front();
                _offsetsReceived.pop_front();
            }
            else if (minOffsetToCommit < expectedOffset)
            {
                // Inconsist error (might be caused by duplicated ack)
                KAFKA_API_LOG(Log::Level::Err, "Got invalid ack offset[%lld]! Even smaller than expected[%lld]! %s", minOffsetToCommit, expectedOffset, (_partitionInfo.empty() ? "" : _partitionInfo.c_str()));
                _offsetsToCommit.pop_front();
            }
            else
            {
                break;
            }
        } while (!_offsetsToCommit.empty());
    }

    /**
     * \brief Pop the offset which is ready for the consumer (if any).
     */
    Optional<Offset> popOffsetToCommit()
    {
        Optional<Offset> ret;
        if (_committed != _toCommit)
        {
            ret = _committed = _toCommit;
        }
        return ret;
    }

    /**
     * \brief Return the offset last popped.
     */
    Optional<Offset> lastPoppedOffset()
    {
        Optional<Offset> ret;
        if (_committed != INVALID_OFFSET)
        {
            ret = _committed;
        }
        return ret;
    }

private:
    std::deque<Offset> _offsetsReceived;
    Heap<Offset>       _offsetsToCommit;
    Offset             _toCommit        = {INVALID_OFFSET};
    Offset             _committed       = {INVALID_OFFSET};
    std::string        _partitionInfo;

    static constexpr Offset INVALID_OFFSET = -1;
};

} // end of KAFKA_API

