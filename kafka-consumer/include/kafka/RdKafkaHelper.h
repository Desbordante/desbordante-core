#pragma once

#include "kafka/Project.h"

#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <memory>

namespace KAFKA_API {

// define smart pointers for rk_kafka_xxx datatypes

struct RkQueueDeleter { void operator()(rd_kafka_queue_t* p) { rd_kafka_queue_destroy(p); } };
using rd_kafka_queue_unique_ptr = std::unique_ptr<rd_kafka_queue_t, RkQueueDeleter>;

struct RkEventDeleter { void operator()(rd_kafka_event_t* p) { rd_kafka_event_destroy(p); } };
using rd_kafka_event_unique_ptr = std::unique_ptr<rd_kafka_event_t, RkEventDeleter>;

struct RkTopicDeleter { void operator()(rd_kafka_topic_t* p) { rd_kafka_topic_destroy(p); } };
using rd_kafka_topic_unique_ptr = std::unique_ptr<rd_kafka_topic_t, RkTopicDeleter>;

struct RkTopicPartitionListDeleter { void operator()(rd_kafka_topic_partition_list_t* p) { rd_kafka_topic_partition_list_destroy(p); } };
using rd_kafka_topic_partition_list_unique_ptr = std::unique_ptr<rd_kafka_topic_partition_list_t, RkTopicPartitionListDeleter>;

struct RkConfDeleter { void operator()(rd_kafka_conf_t* p) { rd_kafka_conf_destroy(p); } };
using rd_kafka_conf_unique_ptr = std::unique_ptr<rd_kafka_conf_t, RkConfDeleter>;

struct RkMetadataDeleter { void operator()(const rd_kafka_metadata_t* p) { rd_kafka_metadata_destroy(p); } };
using rd_kafka_metadata_unique_ptr = std::unique_ptr<const rd_kafka_metadata_t, RkMetadataDeleter>;

struct RkDeleter { void operator()(rd_kafka_t* p) { rd_kafka_destroy(p); } };
using rd_kafka_unique_ptr = std::unique_ptr<rd_kafka_t, RkDeleter>;

struct RkNewTopicDeleter { void operator()(rd_kafka_NewTopic_t* p) { rd_kafka_NewTopic_destroy(p); } };
using rd_kafka_NewTopic_unique_ptr = std::unique_ptr<rd_kafka_NewTopic_t, RkNewTopicDeleter>;

struct RkDeleteTopicDeleter { void operator()(rd_kafka_DeleteTopic_t* p) { rd_kafka_DeleteTopic_destroy(p); } };
using rd_kafka_DeleteTopic_unique_ptr = std::unique_ptr<rd_kafka_DeleteTopic_t, RkDeleteTopicDeleter>;

struct RkDeleteRecordsDeleter { void operator()(rd_kafka_DeleteRecords_t* p) { rd_kafka_DeleteRecords_destroy(p); } };
using rd_kafka_DeleteRecords_unique_ptr = std::unique_ptr<rd_kafka_DeleteRecords_t, RkDeleteRecordsDeleter>;

struct RkConsumerGroupMetadataDeleter { void operator()(rd_kafka_consumer_group_metadata_t* p) { rd_kafka_consumer_group_metadata_destroy(p) ; } };
using rd_kafka_consumer_group_metadata_unique_ptr = std::unique_ptr<rd_kafka_consumer_group_metadata_t, RkConsumerGroupMetadataDeleter>;

inline void RkErrorDeleter(rd_kafka_error_t* p) { rd_kafka_error_destroy(p); }
using rd_kafka_error_shared_ptr = std::shared_ptr<rd_kafka_error_t>;

// Convert from rd_kafka_xxx datatypes
inline TopicPartitionOffsets getTopicPartitionOffsets(const rd_kafka_topic_partition_list_t* rk_tpos)
{
    TopicPartitionOffsets ret;
    int count = rk_tpos ? rk_tpos->cnt : 0;
    for (int i = 0; i < count; ++i)
    {
        const Topic     t = rk_tpos->elems[i].topic;
        const Partition p = rk_tpos->elems[i].partition;
        const Offset    o = rk_tpos->elems[i].offset;

        ret[TopicPartition(t, p)] = o;
    }
    return ret;
}

inline Topics getTopics(const rd_kafka_topic_partition_list_t* rk_topics)
{
    Topics result;
    for (int i = 0; i < (rk_topics ? rk_topics->cnt : 0); ++i)
    {
        result.insert(rk_topics->elems[i].topic);
    }
    return result;
}

inline TopicPartitions getTopicPartitions(const rd_kafka_topic_partition_list_t* rk_tpos)
{
    TopicPartitions result;
    for (int i = 0; i < (rk_tpos ? rk_tpos->cnt : 0); ++i)
    {
        result.insert(TopicPartition{rk_tpos->elems[i].topic, rk_tpos->elems[i].partition});
    }
    return result;
}

// Convert to rd_kafka_xxx datatypes
inline rd_kafka_topic_partition_list_t* createRkTopicPartitionList(const TopicPartitionOffsets& tpos)
{
    rd_kafka_topic_partition_list_t* rk_tpos = rd_kafka_topic_partition_list_new(static_cast<int>(tpos.size()));
    for (const auto& tp_o: tpos)
    {
        const auto& tp = tp_o.first;
        const auto& o  = tp_o.second;
        rd_kafka_topic_partition_t* rk_tp = rd_kafka_topic_partition_list_add(rk_tpos, tp.first.c_str(), tp.second);
        rk_tp->offset = o;
    }
    return rk_tpos;
}

inline rd_kafka_topic_partition_list_t* createRkTopicPartitionList(const TopicPartitions& tps)
{
    TopicPartitionOffsets tpos;
    for (const auto& tp: tps)
    {
        tpos[TopicPartition(tp.first, tp.second)] = RD_KAFKA_OFFSET_INVALID;
    }
    return createRkTopicPartitionList(tpos);
}

inline rd_kafka_topic_partition_list_t* createRkTopicPartitionList(const Topics& topics)
{
    TopicPartitionOffsets tpos;
    for (const auto& topic: topics)
    {
        tpos[TopicPartition(topic, RD_KAFKA_PARTITION_UA)] = RD_KAFKA_OFFSET_INVALID;
    }
    return createRkTopicPartitionList(tpos);
}

} // end of KAFKA_API

