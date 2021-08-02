#pragma once

#include "kafka/Project.h"

#include "kafka/Consumer.h"
#include "kafka/ConsumerConfig.h"
#include "kafka/ConsumerRecord.h"
#include "kafka/KafkaClient.h"

#include "librdkafka/rdkafka.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iterator>
#include <memory>


namespace KAFKA_API {

/**
 * The base class for KafkaAutoCommitConsumer and KafkaManualCommitConsumer.
 */
class KafkaConsumer: public KafkaClient
{
protected:
    // Default value for property "max.poll.records" (which is same with Java API)
    static const constexpr char* DEFAULT_MAX_POLL_RECORDS_VALUE = "500";

    enum class OffsetCommitOption { Auto, Manual };

    // Constructor
    KafkaConsumer(const Properties& properties, KafkaConsumer::OffsetCommitOption offsetCommitOption)
        : KafkaClient(ClientType::KafkaConsumer, properties, registerConfigCallbacks, {ConsumerConfig::MAX_POLL_RECORDS}),
          _offsetCommitOption(offsetCommitOption)
    {
        auto propStr = properties.toString();
        KAFKA_API_DO_LOG(Log::Level::Info, "initializes with properties[%s]", propStr.c_str());

        // Pick up the MAX_POLL_RECORDS configuration
        auto maxPollRecords = properties.getProperty(ConsumerConfig::MAX_POLL_RECORDS);
        assert(maxPollRecords);
        _maxPollRecords = std::stoi(*maxPollRecords);

        // Fetch groupId from configuration
        auto groupId = properties.getProperty(ConsumerConfig::GROUP_ID);
        assert(groupId);
        setGroupId(*groupId);

        // Redirect the reply queue (to the client group queue)
        rd_kafka_resp_err_t err = rd_kafka_poll_set_consumer(getClientHandle());
        KAFKA_THROW_IF_WITH_RESP_ERROR(err);

        // Initialize message-fetching queue
        _rk_queue.reset(rd_kafka_queue_get_consumer(getClientHandle()));
    }

public:
    /**
     * To get group ID.
     */
    std::string getGroupId() const                 { return _groupId; }

    /**
     * To set group ID. The group ID is mandatory for a Consumer.
     */
    void        setGroupId(const std::string& id)  { _groupId = id; }

    /**
     * Subscribe to the given list of topics to get dynamically assigned partitions.
     * An exception would be thrown if assign is called previously (without a subsequent call to unsubscribe())
     */
    void subscribe(const Topics&               topics,
                   Consumer::RebalanceCallback cb      = Consumer::NullRebalanceCallback,
                   std::chrono::milliseconds   timeout = std::chrono::milliseconds(DEFAULT_SUBSCRIBE_TIMEOUT_MS));
    /**
     * Get the current subscription.
     */
    Topics subscription() const;

    /**
     * Unsubscribe from topics currently subscribed.
     */
    void unsubscribe(std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_UNSUBSCRIBE_TIMEOUT_MS));

    /**
     * Manually assign a list of partitions to this consumer.
     * An exception would be thrown if subscribe is called previously (without a subsequent call to unsubscribe())
     */
    void assign(const TopicPartitions& tps);

    /**
     * Get the set of partitions currently assigned to this consumer.
     */
    TopicPartitions assignment() const;

    // Seek & Position
    /**
     * Overrides the fetch offsets that the consumer will use on the next poll(timeout).
     * If this API is invoked for the same partition more than once, the latest offset will be used on the next poll().
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__TIMED_OUT:         Operation timed out
     *   - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: Invalid partition
     *   - RD_KAFKA_RESP_ERR__STATE:             Invalid broker state
     */
    void seek(const TopicPartition& tp, Offset o, std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_SEEK_TIMEOUT_MS));

    /**
     * Seek to the first offset for each of the given partitions.
     * This function evaluates lazily, seeking to the first offset in all partitions only when poll(long) or position(TopicPartition) are called.
     * If no partitions are provided, seek to the first offset for all of the currently assigned partitions.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__TIMED_OUT:         Operation timed out
     *   - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: Invalid partition
     *   - RD_KAFKA_RESP_ERR__STATE:             Invalid broker state
     */
    void seekToBeginning(const TopicPartitions& tps,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_SEEK_TIMEOUT_MS)) { seekToBeginningOrEnd(tps, true, timeout); }
    void seekToBeginning(std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_SEEK_TIMEOUT_MS)) { seekToBeginningOrEnd(_assignment, true, timeout); }

    /**
     * Seek to the last offset for each of the given partitions.
     * This function evaluates lazily, seeking to the final offset in all partitions only when poll(long) or position(TopicPartition) are called.
     * If no partitions are provided, seek to the first offset for all of the currently assigned partitions.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__TIMED_OUT:         Operation timed out
     *   - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: Invalid partition
     *   - RD_KAFKA_RESP_ERR__STATE:             Invalid broker state
     */
    void seekToEnd(const TopicPartitions& tps,
                   std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_SEEK_TIMEOUT_MS)) { seekToBeginningOrEnd(tps, false, timeout); }
    void seekToEnd(std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_SEEK_TIMEOUT_MS)) { seekToBeginningOrEnd(_assignment, false, timeout); }

    /**
     * Get the offset of the next record that will be fetched (if a record with that offset exists).
     */
    Offset position(const TopicPartition& tp) const;

    /**
     * Get the first offset for the given partitions.
     * This method does not change the current consumer position of the partitions.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__FAIL:  Generic failure
     */
    std::map<TopicPartition, Offset> beginningOffsets(const TopicPartitions& tps) const { return getOffsets(tps, true); }

    /**
     * Get the last offset for the given partitions.  The last offset of a partition is the offset of the upcoming message, i.e. the offset of the last available message + 1.
     * This method does not change the current consumer position of the partitions.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__FAIL:  Generic failure
     */
    std::map<TopicPartition, Offset> endOffsets(const TopicPartitions& tps) const { return getOffsets(tps, false); }

    /**
     * Get the offsets for the given partitions by time-point.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__TIMED_OUT:           Not all offsets could be fetched in time.
     *   - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION:   All partitions are unknown.
     *   - RD_KAFKA_RESP_ERR_LEADER_NOT_AVAILABLE: Unable to query leaders from the given partitions.
     */
    std::map<TopicPartition, Offset> offsetsForTime(const TopicPartitions& tps,
                                                    std::chrono::time_point<std::chrono::system_clock> timepoint,
                                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_QUERY_TIMEOUT_MS)) const;

    /**
     * Get the last committed offset for the given partition (whether the commit happened by this process or another).This offset will be used as the position for the consumer in the event of a failure.
     * This call will block to do a remote call to get the latest committed offsets from the server.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG:  Invalid partition
     */
    Offset committed(const TopicPartition& tp);

    /**
     * Fetch data for the topics or partitions specified using one of the subscribe/assign APIs.
     * Returns the polled records.
     * Note: 1) The result could be fetched through ConsumerRecord (with member function `error`).
     *       2) Make sure the `ConsumerRecord` be destructed before the `KafkaConsumer.close()`.
     */
    std::vector<ConsumerRecord> poll(std::chrono::milliseconds timeout);

    /**
     * Fetch data for the topics or partitions specified using one of the subscribe/assign APIs.
     * Returns the number of polled records (which have been saved into parameter `output`).
     * Note: 1) The result could be fetched through ConsumerRecord (with member function `error`).
     *       2) Make sure the `ConsumerRecord` be destructed before the `KafkaConsumer.close()`.
     */
    std::size_t poll(std::chrono::milliseconds timeout, std::vector<ConsumerRecord>& output);

    /**
     * Suspend fetching from the requested partitions. Future calls to poll() will not return any records from these partitions until they have been resumed using resume().
     * Note: 1) After pausing, the application still need to call `poll()` at regular intervals.
     *       2) This method does not affect partition subscription/assignment (i.e, pause fetching from partitions would not trigger a rebalance, since the consumer is still alive).
     *       3) If none of the provided partitions is assigned to this consumer, an exception would be thrown.
     * Throws KafkaException with error:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG: Invalid arguments
     */
    void pause(const TopicPartitions& tps);

    /**
     * Suspend fetching from all assigned partitions. Future calls to poll() will not return any records until they have been resumed using resume().
     * Note: This method does not affect partition subscription/assignment.
     */
    void pause();

    /**
     * Resume specified partitions which have been paused with pause(). New calls to poll() will return records from these partitions if there are any to be fetched.
     * Note: If the partitions were not previously paused, this method is a no-op.
     */
    void resume(const TopicPartitions& tps);

    /**
     * Resume all partitions which have been paused with pause(). New calls to poll() will return records from these partitions if there are any to be fetched.
     */
    void resume();

    /**
     * Return the current group metadata associated with this consumer.
     */
    Consumer::ConsumerGroupMetadata groupMetadata();


protected:
    static const constexpr char* ENABLE_AUTO_OFFSET_STORE = "enable.auto.offset.store";
    static const constexpr char* ENABLE_AUTO_COMMIT       = "enable.auto.commit";
    static const constexpr char* AUTO_COMMIT_INTERVAL_MS  = "auto.commit.interval.ms";

#if __cplusplus >= 201703L
    static constexpr int DEFAULT_SUBSCRIBE_TIMEOUT_MS   = 30000;
    static constexpr int DEFAULT_UNSUBSCRIBE_TIMEOUT_MS = 10000;
    static constexpr int DEFAULT_QUERY_TIMEOUT_MS       = 10000;
    static constexpr int DEFAULT_SEEK_TIMEOUT_MS        = 10000;
    static constexpr int SEEK_RETRY_INTERVAL_MS         = 5000;
#else
    enum { DEFAULT_SUBSCRIBE_TIMEOUT_MS   = 30000 };
    enum { DEFAULT_UNSUBSCRIBE_TIMEOUT_MS = 10000 };
    enum { DEFAULT_QUERY_TIMEOUT_MS       = 10000 };
    enum { DEFAULT_SEEK_TIMEOUT_MS        = 10000 };
    enum { SEEK_RETRY_INTERVAL_MS         = 5000  };
#endif

    const OffsetCommitOption _offsetCommitOption;

    enum class CommitType { Sync, Async };
    void commit(const TopicPartitionOffsets& tpos, CommitType type);

    void close();

    // Offset Commit Callback (for librdkafka)
    static void offsetCommitCallback(rd_kafka_t* rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* rk_tpos, void* opaque);

    // Validate properties (and fix it if necesary)
    static Properties validateAndReformProperties(const Properties& origProperties);

private:
    void commitStoredOffsetsIfNecessary(CommitType type);
    void storeOffsetsIfNecessary(const std::vector<ConsumerRecord>& records);

    void seekToBeginningOrEnd(const TopicPartitions& tps, bool toBeginning, std::chrono::milliseconds timeout);
    std::map<TopicPartition, Offset> getOffsets(const TopicPartitions& tps, bool atBeginning) const;

    enum class PartitionsRebalanceEvent { Assign, Revoke, IncrementalAssign, IncrementalUnassign };
    void changeAssignment(PartitionsRebalanceEvent event, const TopicPartitions& tps);

    std::string  _groupId;

    unsigned int _maxPollRecords = 500; // Default value for batch-poll

    rd_kafka_queue_unique_ptr _rk_queue;

    // Save assignment info (from "assign()" call or rebalance callback) locally, to accelerate seeking procedure
    TopicPartitions _assignment;
    // Assignment from user's input, -- by calling "assign()"
    TopicPartitions _userAssignment;

    // The offsets to store (and commit later)
    std::map<TopicPartition, Offset> _offsetsToStore;

    // Register Callbacks for rd_kafka_conf_t
    static void registerConfigCallbacks(rd_kafka_conf_t* conf);

    void pollMessages(int timeoutMs, std::vector<ConsumerRecord>& output);

    enum class PauseOrResumeOperation { Pause, Resume };
    void pauseOrResumePartitions(const TopicPartitions& tps, PauseOrResumeOperation op);

    // Rebalance Callback (for librdkafka)
    static void rebalanceCallback(rd_kafka_t* rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* partitions, void* opaque);
    // Rebalance Callback (for class instance)
    void onRebalance(rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* rk_partitions);

    Consumer::RebalanceCallback _rebalanceCb;
};


// Validate properties (and fix it if necesary)
inline Properties
KafkaConsumer::validateAndReformProperties(const Properties& origProperties)
{
    // Let the base class validate first
    Properties properties = KafkaClient::validateAndReformProperties(origProperties);

    // If no "group.id" configured, generate a random one for user
    if (!properties.getProperty(ConsumerConfig::GROUP_ID))
    {
        properties.put(ConsumerConfig::GROUP_ID, Utility::getRandomString());
    }

    // If no "max.poll.records" configured, use a default value
    if (!properties.getProperty(ConsumerConfig::MAX_POLL_RECORDS))
    {
        properties.put(ConsumerConfig::MAX_POLL_RECORDS, DEFAULT_MAX_POLL_RECORDS_VALUE);
    }

    // We want to customize the auto-commit behavior, with librdkafka's configuration disabled
    properties.put(ENABLE_AUTO_COMMIT,       "false");
    properties.put(AUTO_COMMIT_INTERVAL_MS,  "0");

    return properties;
}

// Register Callbacks for rd_kafka_conf_t
inline void
KafkaConsumer::registerConfigCallbacks(rd_kafka_conf_t* conf)
{
    // Rebalance Callback
    // would turn off librdkafka's automatic partition assignment/revocation
    rd_kafka_conf_set_rebalance_cb(conf, KafkaConsumer::rebalanceCallback);
}

inline void
KafkaConsumer::close()
{
    _opened = false;

    try
    {
        // Commit the offsets for these messages which had been polled last time (for KafkaAutoCommitConsumer)
        commitStoredOffsetsIfNecessary(CommitType::Sync);
    }
    catch(const KafkaException& e)
    {
        KAFKA_API_DO_LOG(Log::Level::Err, "met error[%s] while closing", e.what());
    }

    rd_kafka_consumer_close(getClientHandle());

    while (rd_kafka_outq_len(getClientHandle()))
    {
        rd_kafka_poll(getClientHandle(), KafkaClient::TIMEOUT_INFINITE);
    }

    KAFKA_API_DO_LOG(Log::Level::Info, "closed");
}


// Subscription
inline void
KafkaConsumer::subscribe(const Topics& topics, Consumer::RebalanceCallback cb, std::chrono::milliseconds timeout)
{
    std::string topicsStr = toString(topics);

    if (!_userAssignment.empty())
    {
        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__FAIL, "Unexpected Operation! Once assign() was used, subscribe() should not be called any more!");
    }

    KAFKA_API_DO_LOG(Log::Level::Info, "will subscribe, topics[%s]", topicsStr.c_str());

    _rebalanceCb = std::move(cb);

    auto rk_topics = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList(topics));

    rd_kafka_resp_err_t err = rd_kafka_subscribe(getClientHandle(), rk_topics.get());
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    // The rebalcance callback (e.g. "assign", etc) would be served during the time (within this thread)
    rd_kafka_poll(getClientHandle(), static_cast<int>(timeout.count()));        // NOLLINT

    KAFKA_API_DO_LOG(Log::Level::Info, "subscribed, topics[%s]", topicsStr.c_str());
}

inline void
KafkaConsumer::unsubscribe(std::chrono::milliseconds timeout)
{
    KAFKA_API_DO_LOG(Log::Level::Info, "will unsubscribe");

    rd_kafka_resp_err_t err = rd_kafka_unsubscribe(getClientHandle());
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    // The rebalcance callback (e.g. "assign", etc) would be served during the time (within this thread)
    rd_kafka_poll(getClientHandle(), static_cast<int>(timeout.count()));        // NOLLINT

    KAFKA_API_DO_LOG(Log::Level::Info, "unsubscribed");
}

inline Topics
KafkaConsumer::subscription() const
{
    rd_kafka_topic_partition_list_t* raw_topics = nullptr;
    rd_kafka_resp_err_t err = rd_kafka_subscription(getClientHandle(), &raw_topics);
    auto rk_topics = rd_kafka_topic_partition_list_unique_ptr(raw_topics);

    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    return getTopics(rk_topics.get());
}

inline void
KafkaConsumer::changeAssignment(PartitionsRebalanceEvent event, const TopicPartitions& tps)
{
    auto rk_tps = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList(tps));

    std::unique_ptr<Error> result;
    switch (event)
    {
        case PartitionsRebalanceEvent::Assign:
        case PartitionsRebalanceEvent::Revoke:
            result = std::make_unique<SimpleError>(rd_kafka_assign(getClientHandle(), (rk_tps->cnt > 0) ? rk_tps.get() : nullptr));
            break;
        case PartitionsRebalanceEvent::IncrementalAssign:
            result = std::make_unique<Error>(rd_kafka_incremental_assign(getClientHandle(), rk_tps.get()));
            break;
        case PartitionsRebalanceEvent::IncrementalUnassign:
            result = std::make_unique<Error>(rd_kafka_incremental_unassign(getClientHandle(), rk_tps.get()));
            break;
    }

    KAFKA_THROW_IF_WITH_ERROR(*result);
    _assignment = tps;
}

// Assign Topic-Partitions
inline void
KafkaConsumer::assign(const TopicPartitions& tps)
{
    if (!subscription().empty())
    {
        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__FAIL, "Unexpected Operation! Once subscribe() was used, assign() should not be called any more!");
    }

    _userAssignment = tps;

    changeAssignment(PartitionsRebalanceEvent::Assign, tps);
}

// Assignment
inline TopicPartitions
KafkaConsumer::assignment() const
{
    rd_kafka_topic_partition_list_t* raw_tps = nullptr;
    rd_kafka_resp_err_t err = rd_kafka_assignment(getClientHandle(), &raw_tps);

    auto rk_tps = rd_kafka_topic_partition_list_unique_ptr(raw_tps);

    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    return getTopicPartitions(rk_tps.get());
}


// Seek & Position
inline void
KafkaConsumer::seek(const TopicPartition& tp, Offset o, std::chrono::milliseconds timeout)
{
    std::string tpStr = toString(tp);
    KAFKA_API_DO_LOG(Log::Level::Info, "will seek with topic-partition[%s], offset[%d]", tpStr.c_str(), o);

    auto rkt = rd_kafka_topic_unique_ptr(rd_kafka_topic_new(getClientHandle(), tp.first.c_str(), nullptr));
    if (!rkt)
    {
        KAFKA_THROW_RESP_ERROR(rd_kafka_last_error());
    }

    const auto end = std::chrono::steady_clock::now() + timeout;

    rd_kafka_resp_err_t err = RD_KAFKA_RESP_ERR_NO_ERROR;
    do
    {
        err = rd_kafka_seek(rkt.get(), tp.second, o, SEEK_RETRY_INTERVAL_MS);
        if (err != RD_KAFKA_RESP_ERR__STATE && err != RD_KAFKA_RESP_ERR__TIMED_OUT && err != RD_KAFKA_RESP_ERR__OUTDATED)
        {
            break;
        }

        // If the "seek" was called just after "assign", there's a chance that the toppar's "fetch_state" (async setted) was not ready yes.
        // If that's the case, we would retry again (normally, just after a very short while, the "seek" would succeed)
        std::this_thread::yield();
    } while (std::chrono::steady_clock::now() < end);

    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    KAFKA_API_DO_LOG(Log::Level::Info, "seeked with topic-partition[%s], offset[%d]", tpStr.c_str(), o);
}

inline void
KafkaConsumer::seekToBeginningOrEnd(const TopicPartitions& tps, bool toBeginning, std::chrono::milliseconds timeout)
{
    for (const auto& tp: tps)
    {
        seek(tp, (toBeginning ? RD_KAFKA_OFFSET_BEGINNING : RD_KAFKA_OFFSET_END), timeout);
    }
}

inline Offset
KafkaConsumer::position(const TopicPartition& tp) const
{
    auto rk_tp = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList({tp}));

    rd_kafka_resp_err_t err = rd_kafka_position(getClientHandle(), rk_tp.get());
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    return rk_tp->elems[0].offset;
}

inline std::map<TopicPartition, Offset>
KafkaConsumer::offsetsForTime(const TopicPartitions& tps,
                              std::chrono::time_point<std::chrono::system_clock> timepoint,
                              std::chrono::milliseconds timeout) const
{
    if (tps.empty()) return TopicPartitionOffsets();

    auto msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count();

    auto rk_tpos = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList(tps));

    for (int i = 0; i < rk_tpos->cnt; ++i)
    {
        rd_kafka_topic_partition_t& rk_tp = rk_tpos->elems[i];
        // Here the `msSinceEpoch` would be overridden by the offset result (after called by `rd_kafka_offsets_for_times`)
        rk_tp.offset = msSinceEpoch;
    }

    rd_kafka_resp_err_t err = rd_kafka_offsets_for_times(getClientHandle(), rk_tpos.get(), static_cast<int>(timeout.count()));      // NOLINT
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    auto results = getTopicPartitionOffsets(rk_tpos.get());

    // Remove invalid results (which are not updated with an valid offset)
    for (auto it = results.begin(); it != results.end(); )
    {
        it = ((it->second == msSinceEpoch) ? results.erase(it) : std::next(it));
    }

    return results;
}

inline std::map<TopicPartition, Offset>
KafkaConsumer::getOffsets(const TopicPartitions& tps, bool atBeginning) const
{
    std::map<TopicPartition, Offset> result;

    for (const auto& tp: tps)
    {
        Offset beginning{}, end{};
        rd_kafka_resp_err_t err = rd_kafka_query_watermark_offsets(getClientHandle(), tp.first.c_str(), tp.second, &beginning, &end, 0);
        KAFKA_THROW_IF_WITH_RESP_ERROR(err);

        result[tp] = (atBeginning ? beginning : end);
    }

    return result;
}

// Commit
inline void
KafkaConsumer::commit(const TopicPartitionOffsets& tpos, CommitType type)
{
    auto rk_tpos = rd_kafka_topic_partition_list_unique_ptr(tpos.empty() ? nullptr : createRkTopicPartitionList(tpos));

    rd_kafka_resp_err_t err = rd_kafka_commit(getClientHandle(), rk_tpos.get(), type == CommitType::Async ? 1 : 0);
    // No stored offset to commit (it might happen and should not be treated as a mistake)
    if (tpos.empty() && err == RD_KAFKA_RESP_ERR__NO_OFFSET)
    {
        err = RD_KAFKA_RESP_ERR_NO_ERROR;
    }

    KAFKA_THROW_IF_WITH_RESP_ERROR(err);
}

// Fetch committed offset
inline Offset
KafkaConsumer::committed(const TopicPartition& tp)
{
    auto rk_tps = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList({tp}));

    rd_kafka_resp_err_t err = rd_kafka_committed(getClientHandle(), rk_tps.get(), TIMEOUT_INFINITE);
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    return rk_tps->elems[0].offset;
}

// Commit stored offsets
inline void
KafkaConsumer::commitStoredOffsetsIfNecessary(CommitType type)
{
    if (_offsetCommitOption == OffsetCommitOption::Auto && !_offsetsToStore.empty())
    {
        for (auto& o: _offsetsToStore)
        {
            ++o.second;
        }
        commit(_offsetsToStore, type);
        _offsetsToStore.clear();
    }
}

// Store offsets
inline void
KafkaConsumer::storeOffsetsIfNecessary(const std::vector<ConsumerRecord>& records)
{
    if (_offsetCommitOption == OffsetCommitOption::Auto)
    {
        for (const auto& record: records)
        {
            _offsetsToStore[TopicPartition(record.topic(), record.partition())] = record.offset();
        }
    }
}

// Fetch messages (internally used)
inline void
KafkaConsumer::pollMessages(int timeoutMs, std::vector<ConsumerRecord>& output)
{
    // Commit the offsets for these messages which had been polled last time (for KafkaAutoCommitConsumer)
    commitStoredOffsetsIfNecessary(CommitType::Async);

    // Poll messages with librdkafka's API
    std::vector<rd_kafka_message_t*> msgPtrArray(_maxPollRecords);
    std::size_t msgReceived = rd_kafka_consume_batch_queue(_rk_queue.get(), timeoutMs, msgPtrArray.data(), _maxPollRecords);

    // Wrap messages with ConsumerRecord
    output.clear();
    output.reserve(msgReceived);
    std::for_each(&msgPtrArray[0], &msgPtrArray[msgReceived], [&output](rd_kafka_message_t* rkMsg) { output.emplace_back(rkMsg); });

    // Store the offsets for all these polled messages (for KafkaAutoCommitConsumer)
    storeOffsetsIfNecessary(output);
}

// Fetch messages (return via return value)
inline std::vector<ConsumerRecord>
KafkaConsumer::poll(std::chrono::milliseconds timeout)
{
    std::vector<ConsumerRecord> result;
    poll(timeout, result);
    return result;
}

// Fetch messages (return via input parameter)
inline std::size_t
KafkaConsumer::poll(std::chrono::milliseconds timeout, std::vector<ConsumerRecord>& output)
{
    pollMessages(convertMsDurationToInt(timeout), output);
    return output.size();
}

inline void
KafkaConsumer::pauseOrResumePartitions(const TopicPartitions& tps, PauseOrResumeOperation op)
{
    auto rk_tpos = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList(tps));

    rd_kafka_resp_err_t err = (op == PauseOrResumeOperation::Pause) ?
                              rd_kafka_pause_partitions(getClientHandle(), rk_tpos.get()) : rd_kafka_resume_partitions(getClientHandle(), rk_tpos.get());
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);

    const char* opString = (op == PauseOrResumeOperation::Pause) ? "pause" : "resume";
    int cnt = 0;
    for (int i = 0; i < rk_tpos->cnt; ++i)
    {
        const rd_kafka_topic_partition_t& rk_tp = rk_tpos->elems[i];
        if (rk_tp.err != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            KAFKA_API_DO_LOG(Log::Level::Err, "%s topic-partition[%s-%d] error[%s]", opString, rk_tp.topic, rk_tp.partition, rd_kafka_err2str(rk_tp.err));
        }
        else
        {
            KAFKA_API_DO_LOG(Log::Level::Info, "%sd topic-partition[%s-%d]", opString, rk_tp.topic, rk_tp.partition, rd_kafka_err2str(rk_tp.err));
            ++cnt;
        }
    }

    if (cnt == 0 && op == PauseOrResumeOperation::Pause)
    {
        std::string errMsg = std::string("No partition could be ") + opString + std::string("d among TopicPartitions[") + toString(tps) + std::string("]");
        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG, errMsg);
    }
}

inline void
KafkaConsumer::pause(const TopicPartitions& tps)
{
    pauseOrResumePartitions(tps, PauseOrResumeOperation::Pause);
}

inline void
KafkaConsumer::pause()
{
    pause(_assignment);
}

inline void
KafkaConsumer::resume(const TopicPartitions& tps)
{
    pauseOrResumePartitions(tps, PauseOrResumeOperation::Resume);
}

inline void
KafkaConsumer::resume()
{
    resume(_assignment);
}

// Rebalance Callback (for class instance)
inline void
KafkaConsumer::onRebalance(rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* rk_partitions)
{
    TopicPartitions tps = getTopicPartitions(rk_partitions);
    std::string tpsStr = toString(tps);

    if (err != RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS && err != RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS)
    {
        KAFKA_API_DO_LOG(Log::Level::Err, "unknown re-balance event[%d], topic-partitions[%s]",  err, tpsStr.c_str());
        return;
    }

    const char* protocol = rd_kafka_rebalance_protocol(getClientHandle());
    bool cooperativeEnabled = (protocol && (std::string(protocol) == "COOPERATIVE"));

    KAFKA_API_DO_LOG(Log::Level::Info, "re-balance event triggered[%s], cooperative[%s], topic-partitions[%s]",
                     err == RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS ? "ASSIGN_PARTITIONS" : "REVOKE_PARTITIONS",
                     cooperativeEnabled ? "enabled" : "disabled",
                     tpsStr.c_str());

    PartitionsRebalanceEvent event = (err == RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS ?
                                         (cooperativeEnabled ? PartitionsRebalanceEvent::IncrementalAssign : PartitionsRebalanceEvent::Assign)
                                         : (cooperativeEnabled ? PartitionsRebalanceEvent::IncrementalUnassign : PartitionsRebalanceEvent::Revoke));

    if (err == RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS)
    {
        changeAssignment(event, tps);
    }

    if (_rebalanceCb)
    {
        _rebalanceCb(err == RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS ? Consumer::RebalanceEventType::PartitionsAssigned : Consumer::RebalanceEventType::PartitionsRevoked,
                     tps);
    }

    if (err == RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS)
    {
        changeAssignment(event, cooperativeEnabled ? tps : TopicPartitions{});
    }
}

// Rebalance Callback (for librdkafka)
inline void
KafkaConsumer::rebalanceCallback(rd_kafka_t* rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* partitions, void* /* opaque */)
{
    KafkaClient& client   = kafkaClient(rk);
    auto&        consumer = dynamic_cast<KafkaConsumer&>(client);
    consumer.onRebalance(err, partitions);
}

// Offset Commit Callback (for librdkafka)
inline void
KafkaConsumer::offsetCommitCallback(rd_kafka_t* rk, rd_kafka_resp_err_t err, rd_kafka_topic_partition_list_t* rk_tpos, void* opaque)
{
    TopicPartitionOffsets tpos = getTopicPartitionOffsets(rk_tpos);

    if (err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        auto tposStr = toString(tpos);
        kafkaClient(rk).KAFKA_API_DO_LOG(Log::Level::Err, "invoked offset-commit callback. offsets[%s], result[%s]", tposStr.c_str(), rd_kafka_err2str(err));
    }

    auto* cb = static_cast<Consumer::OffsetCommitCallback*>(opaque);
    if (cb && *cb)
    {
        (*cb)(tpos, ErrorCode(err));
    }
    delete cb;
}

inline Consumer::ConsumerGroupMetadata
KafkaConsumer::groupMetadata()
{
    return Consumer::ConsumerGroupMetadata{rd_kafka_consumer_group_metadata(getClientHandle())};
}

/**
 * Automatic-Commit consumer.
 * Whenever you poll, the consumer checks if it is time to commit, and if it is, it will commit the offsets it returned in the last poll.
 */
class KafkaAutoCommitConsumer: public KafkaConsumer
{
public:
    /**
     * The constructor for KafkaAutoCommitConsumer.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG:       Invalid BOOTSTRAP_SERVERS property
     *   - RD_KAFKA_RESP_ERR__CRIT_SYS_RESOURCE: Fail to create internal threads
     */
    explicit KafkaAutoCommitConsumer(const Properties& properties)
        : KafkaConsumer(KafkaAutoCommitConsumer::validateAndReformProperties(properties), OffsetCommitOption::Auto)
    {
    }

    ~KafkaAutoCommitConsumer() override { if (_opened) close(); }

    /**
     * Close the consumer, waiting for any needed cleanup.
     */
    void close()
    {
        KafkaConsumer::close();
    }

private:
    // Validate properties (and fix it if necesary)
    static Properties validateAndReformProperties(const Properties& origProperties)
    {
        // Let the base class validate first
        Properties properties = KafkaConsumer::validateAndReformProperties(origProperties);

        // Don't "auto-store" offsets (librdkafka's configuration)
        properties.put(ENABLE_AUTO_OFFSET_STORE, "false");

        return properties;
    }
};

/**
 * Manual-Commit consumer.
 * User must use commitSync/commitAsync to commit the offsets manually.
 */
class KafkaManualCommitConsumer: public KafkaConsumer
{
public:
    /**
     * The constructor for KafkaManualCommitConsumer.
     *
     * Options:
     *   - EventsPollingOption::Auto (default) : An internal thread would be started for OffsetCommit callbacks handling.
     *   - EventsPollingOption::Maunal         : User have to call the member function `pollEvents()` to trigger OffsetCommit callbacks.
     *
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG      : Invalid BOOTSTRAP_SERVERS property
     *   - RD_KAFKA_RESP_ERR__CRIT_SYS_RESOURCE: Fail to create internal threads
     */
    explicit KafkaManualCommitConsumer(const Properties&   properties,
                                       EventsPollingOption pollOption = EventsPollingOption::Auto)
        : KafkaConsumer(KafkaManualCommitConsumer::validateAndReformProperties(properties), OffsetCommitOption::Manual)
    {
        _rk_commit_cb_queue.reset(rd_kafka_queue_new(getClientHandle()));

        _pollable = std::make_unique<KafkaClient::PollableCallback<KafkaManualCommitConsumer>>(this, pollCallbacks);
        if (pollOption == EventsPollingOption::Auto)
        {
            _pollThread = std::make_unique<PollThread>(*_pollable);
        }
    }

    ~KafkaManualCommitConsumer() override { if (_opened) close(); }

    /**
     * Close the consumer, waiting for any needed cleanup.
     */
    void close()
    {
        _pollThread.reset(); // Join the polling thread (in case it's running)
        _pollable.reset();

        KafkaConsumer::close();

        rd_kafka_queue_t* queue = getCommitCbQueue();
        while (rd_kafka_queue_length(queue))
        {
            rd_kafka_queue_poll_callback(queue, TIMEOUT_INFINITE);
        }
    }

    /**
     * Commit offsets returned on the last poll() for all the subscribed list of topics and partitions.
     */
    void commitSync();
    /**
     * Commit the specified offsets for the specified records
     */
    void commitSync(const ConsumerRecord& record);
    /**
     * Commit the specified offsets for the specified list of topics and partitions.
     */
    void commitSync(const TopicPartitionOffsets& tpos);
    /**
     * Commit offsets returned on the last poll() for all the subscribed list of topics and partition.
     * Note: If a callback is provided, it's guaranteed to be triggered (before closing the consumer).
     */
    void commitAsync(const Consumer::OffsetCommitCallback& cb = Consumer::NullOffsetCommitCallback);
    /**
     * Commit the specified offsets for the specified records
     * Note: If a callback is provided, it's guaranteed to be triggered (before closing the consumer).
     */
    void commitAsync(const ConsumerRecord& record, const Consumer::OffsetCommitCallback& cb = Consumer::NullOffsetCommitCallback);
    /**
     * Commit the specified offsets for the specified list of topics and partitions to Kafka.
     * Note: If a callback is provided, it's guaranteed to be triggered (before closing the consumer).
     */
    void commitAsync(const TopicPartitionOffsets& tpos, const Consumer::OffsetCommitCallback& cb = Consumer::NullOffsetCommitCallback);

    /**
     * Call the OffsetCommit callbacks (if any)
     * Note: The KafkaManualCommitConsumer MUST be constructed with option `EventsPollingOption::Manual`.
     */
    void pollEvents(std::chrono::milliseconds timeout)
    {
        assert(!_pollThread);

        _pollable->poll(convertMsDurationToInt(timeout));
    }

private:
    rd_kafka_queue_t* getCommitCbQueue() { return _rk_commit_cb_queue.get(); }

    rd_kafka_queue_unique_ptr _rk_commit_cb_queue;

    std::unique_ptr<Pollable>   _pollable;
    std::unique_ptr<PollThread> _pollThread;

    static void pollCallbacks(KafkaManualCommitConsumer* consumer, int timeoutMs)
    {
        rd_kafka_queue_t* queue = consumer->getCommitCbQueue();
        rd_kafka_queue_poll_callback(queue, timeoutMs);
    }

    // Validate properties (and fix it if necesary)
    static Properties validateAndReformProperties(const Properties& origProperties)
    {
        // Let the base class validate first
        Properties properties = KafkaConsumer::validateAndReformProperties(origProperties);

        // Automatically store offset of last message provided to application
        properties.put(ENABLE_AUTO_OFFSET_STORE, "true");

        return properties;
    }
};

inline void
KafkaManualCommitConsumer::commitSync()
{
    commit(TopicPartitionOffsets(), CommitType::Sync);
}

inline void
KafkaManualCommitConsumer::commitSync(const ConsumerRecord& record)
{
    TopicPartitionOffsets tpos;
    // committed offset should be "current-received-offset + 1"
    tpos[TopicPartition(record.topic(), record.partition())] = record.offset() + 1;

    commit(tpos, CommitType::Sync);
}

inline void
KafkaManualCommitConsumer::commitSync(const TopicPartitionOffsets& tpos)
{
    commit(tpos, CommitType::Sync);
}

inline void
KafkaManualCommitConsumer::commitAsync(const TopicPartitionOffsets& tpos, const Consumer::OffsetCommitCallback& cb)
{
    auto rk_tpos = rd_kafka_topic_partition_list_unique_ptr(tpos.empty() ? nullptr : createRkTopicPartitionList(tpos));

    rd_kafka_resp_err_t err = rd_kafka_commit_queue(getClientHandle(), rk_tpos.get(), getCommitCbQueue(), &KafkaConsumer::offsetCommitCallback, new Consumer::OffsetCommitCallback(cb));
    KAFKA_THROW_IF_WITH_RESP_ERROR(err);
}

inline void
KafkaManualCommitConsumer::commitAsync(const ConsumerRecord& record, const Consumer::OffsetCommitCallback& cb)
{
    TopicPartitionOffsets tpos;
    // committed offset should be "current received record's offset" + 1
    tpos[TopicPartition(record.topic(), record.partition())] = record.offset() + 1;
    commitAsync(tpos, cb);
}

inline void
KafkaManualCommitConsumer::commitAsync(const Consumer::OffsetCommitCallback& cb)
{
    commitAsync(TopicPartitionOffsets(), cb);
}

} // end of KAFKA_API

