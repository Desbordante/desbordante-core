#pragma once

#include "kafka/Project.h"

#include "kafka/RdKafkaHelper.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <functional>


namespace KAFKA_API {

namespace Consumer
{
    /**
     * To identify which kind of re-balance event is handling, when the set of partitions assigned to the consumer changes.
     * It's guaranteed that rebalance callback will be called twice (first with PartitionsRevoked, and then with PartitionsAssigned).
     */
    enum class RebalanceEventType { PartitionsAssigned, PartitionsRevoked };

    /**
     * A callback interface that the user can implement to trigger custom actions when the set of partitions assigned to the consumer changes.
     */
    using RebalanceCallback = std::function<void(RebalanceEventType eventType, const TopicPartitions& topicPartitions)>;

    /**
     * Null RebalanceCallback
     */
#if __cplusplus >= 201703L
    const inline RebalanceCallback NullRebalanceCallback = RebalanceCallback{};
#else
    const static RebalanceCallback NullRebalanceCallback = RebalanceCallback{};
#endif

    /**
     * A callback interface that the user can implement to trigger custom actions when a commit request completes.
     */
    using OffsetCommitCallback = std::function<void(const TopicPartitionOffsets& topicPartitionOffsets, std::error_code ec)>;

    /**
     * Null OffsetCommitCallback
     */
#if __cplusplus >= 201703L
    const inline OffsetCommitCallback NullOffsetCommitCallback = OffsetCommitCallback{};
#else
    const static OffsetCommitCallback NullOffsetCommitCallback = OffsetCommitCallback{};
#endif

    /**
     * A metadata struct containing the consumer group information.
     */
    class ConsumerGroupMetadata
    {
    public:
        explicit ConsumerGroupMetadata(rd_kafka_consumer_group_metadata_t* p): _rkConsumerGroupMetadata(p) {}

        const rd_kafka_consumer_group_metadata_t*  rawHandle() const { return _rkConsumerGroupMetadata.get(); }

    private:
        rd_kafka_consumer_group_metadata_unique_ptr _rkConsumerGroupMetadata;
    };

} // end of Consumer

} // end of KAFKA_API

