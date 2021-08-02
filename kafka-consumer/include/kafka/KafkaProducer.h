#pragma once

#include "kafka/Project.h"

#include "kafka/Consumer.h"
#include "kafka/KafkaClient.h"
#include "kafka/Producer.h"
#include "kafka/ProducerConfig.h"
#include "kafka/ProducerRecord.h"
#include "kafka/Timestamp.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <cassert>
#include <future>
#include <memory>
#include <shared_mutex>
#include <unordered_map>


namespace KAFKA_API {

/**
 * The base class for KafkaAsyncProducer and KafkaSyncProducer.
 */
class KafkaProducer: public KafkaClient
{
public:
    /**
     * Invoking this method makes all buffered records immediately available to send, and blocks on the completion of the requests associated with these records.
     *
     * Possible errors:
     *   - RD_KAFKA_RESP_ERR__TIMED_OUT: The `timeout` was reached before all outstanding requests were completed.
     */
    std::error_code flush(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    enum class SendOption { NoCopyRecordValue, ToCopyRecordValue };

    /**
     * Needs to be called before any other methods when the transactional.id is set in the configuration.
     */
    void initTransactions(std::chrono::milliseconds timeout = std::chrono::milliseconds(KafkaProducer::DEFAULT_INIT_TRANSACTIONS_TIMEOUT_MS));

    /**
     * Should be called before the start of each new transaction.
     */
    void beginTransaction();

    /**
     * Commit the ongoing transaction.
     */
    void commitTransaction(std::chrono::milliseconds timeout = std::chrono::milliseconds(KafkaProducer::DEFAULT_COMMIT_TRANSACTION_TIMEOUT_MS));

    /**
     * Abort the ongoing transaction.
     */
    void abortTransaction(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());


    /**
     * Send a list of specified offsets to the consumer group coodinator, and also marks those offsets as part of the current transaction.
     */
    void sendOffsetsToTransaction(const TopicPartitionOffsets& tpos,
                                  const Consumer::ConsumerGroupMetadata& groupMetadata,
                                  std::chrono::milliseconds timeout);

protected:
    explicit KafkaProducer(const Properties& properties)
        : KafkaClient(ClientType::KafkaProducer, properties, registerConfigCallbacks)
    {
        auto propStr = properties.toString();
        KAFKA_API_DO_LOG(Log::Level::Info, "initializes with properties[%s]", propStr.c_str());
    }

    std::error_code close(std::chrono::milliseconds timeout);

    // Define datatypes for "opaque" (as a parameter of rd_kafka_produce), in order to handle the delivery callback
    class DeliveryCbOpaque
    {
    public:
        DeliveryCbOpaque(ProducerRecord::Id id, Producer::Callback cb): _recordId(id), _drCb(std::move(cb)) {}

        void operator()(rd_kafka_t* /*rk*/, const rd_kafka_message_t* rkmsg)
        {
            _drCb(Producer::RecordMetadata{rkmsg, _recordId}, ErrorCode(rkmsg->err));
        }

    private:
        ProducerRecord::Id _recordId;
        Producer::Callback _drCb;
    };

    enum class ActionWhileQueueIsFull { Block, NoBlock };

    rd_kafka_resp_err_t sendMessage(const ProducerRecord&                   record,
                                    std::unique_ptr<DeliveryCbOpaque> opaque,
                                    SendOption                              option,
                                    ActionWhileQueueIsFull                  action);

    static constexpr int CALLBACK_POLLING_INTERVAL_MS = 10;

#if __cplusplus >= 201703L
    static constexpr int DEFAULT_INIT_TRANSACTIONS_TIMEOUT_MS  = 10000;
    static constexpr int DEFAULT_COMMIT_TRANSACTION_TIMEOUT_MS = 10000;
#else
    enum { DEFAULT_INIT_TRANSACTIONS_TIMEOUT_MS  = 10000 };
    enum { DEFAULT_COMMIT_TRANSACTION_TIMEOUT_MS = 10000 };
#endif

    // Validate properties (and fix it if necesary)
    static Properties validateAndReformProperties(const Properties& origProperties);

    // Delivery Callback (for librdkafka)
    static void deliveryCallback(rd_kafka_t* rk, const rd_kafka_message_t* rkmsg, void* opaque);

    // Register Callbacks for rd_kafka_conf_t
    static void registerConfigCallbacks(rd_kafka_conf_t* conf);

#ifdef KAFKA_API_ENABLE_UNIT_TEST_STUBS
public:
    using HandleProduceResponseCb = std::function<rd_kafka_resp_err_t(rd_kafka_t* /*rk*/, int32_t /*brokerid*/, uint64_t /*msgseq*/, rd_kafka_resp_err_t /*err*/)>;

    /**
     * Stub for ProduceResponse handing.
     * Note: Only for internal unit tests
     */
    void stubHandleProduceResponse(HandleProduceResponseCb cb = HandleProduceResponseCb()) { _handleProduceRespCb = std::move(cb); }

private:
    static rd_kafka_resp_err_t handleProduceResponse(rd_kafka_t* rk, int32_t brokerId, uint64_t msgSeq, rd_kafka_resp_err_t err)
    {
        auto* client   = static_cast<KafkaClient*>(rd_kafka_opaque(rk));
        auto* producer = dynamic_cast<KafkaProducer*>(client);
        auto  respCb   = producer->_handleProduceRespCb;
        return respCb ? respCb(rk, brokerId, msgSeq, err) : err;
    }

    HandleProduceResponseCb _handleProduceRespCb;
#endif
};

inline void
KafkaProducer::registerConfigCallbacks(rd_kafka_conf_t* conf)
{
    // Delivery Callback
    rd_kafka_conf_set_dr_msg_cb(conf, deliveryCallback);

#ifdef KAFKA_API_ENABLE_UNIT_TEST_STUBS
    // UT stub for ProduceResponse
    LogBuffer<LOG_BUFFER_SIZE> errInfo;
    if (rd_kafka_conf_set(conf, "ut_handle_ProduceResponse", reinterpret_cast<char*>(&handleProduceResponse), errInfo.str(), errInfo.capacity()))   // NOLINT
    {
        KafkaClient* client = nullptr;
        size_t clientPtrSize = 0;
        if (rd_kafka_conf_get(conf, "opaque", reinterpret_cast<char*>(&client), &clientPtrSize))    // NOLINT
        {
            KAFKA_API_LOG(Log::Level::Crit, "failed to stub ut_handle_ProduceResponse! error[%s]. Meanwhile, failed to get the Kafka client!", errInfo.c_str());
        }
        else
        {
            assert(clientPtrSize == sizeof(client));
            client->KAFKA_API_DO_LOG(Log::Level::Err, "failed to stub ut_handle_ProduceResponse! error[%s]", errInfo.c_str());
        }
    }
#endif
}

inline Properties
KafkaProducer::validateAndReformProperties(const Properties& origProperties)
{
    // Let the base class validate first
    Properties properties = KafkaClient::validateAndReformProperties(origProperties);

    // Check whether it's an available partitioner
    const std::set<std::string> availPartitioners = {"murmur2_random", "murmur2", "random", "consistent", "consistent_random", "fnv1a", "fnv1a_random"};
    auto partitioner = properties.getProperty(ProducerConfig::PARTITIONER);
    if (partitioner && !availPartitioners.count(*partitioner))
    {
        std::string errMsg = "Invalid partitioner [" + *partitioner + "]! Valid options: ";
        bool isTheFirst = true;
        for (const auto& availPartitioner: availPartitioners)
        {
            errMsg += (std::string(isTheFirst ? (isTheFirst = false, "") : ", ") + availPartitioner);
        }
        errMsg += ".";

        KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG, errMsg);
    }

    // For "idempotence" feature
    constexpr int KAFKA_IDEMP_MAX_INFLIGHT = 5;
    const auto enableIdempotence = properties.getProperty(ProducerConfig::ENABLE_IDEMPOTENCE);
    if (enableIdempotence && *enableIdempotence == "true")
    {
        if (const auto maxInFlight = properties.getProperty(ProducerConfig::MAX_IN_FLIGHT))
        {
            if (std::stoi(*maxInFlight) > KAFKA_IDEMP_MAX_INFLIGHT)
            {
                KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG,\
                                     "`max.in.flight` must be set <= " + std::to_string(KAFKA_IDEMP_MAX_INFLIGHT) + " when `enable.idempotence` is `true`");
            }
        }

        if (const auto acks = properties.getProperty(ProducerConfig::ACKS))
        {
            if (*acks != "all" && *acks != "-1")
            {
                KAFKA_THROW_WITH_MSG(RD_KAFKA_RESP_ERR__INVALID_ARG,\
                                     "`acks` must be set to `all`/`-1` when `enable.idempotence` is `true`");
            }
        }
    }

    return properties;
}

// Delivery Callback (for librdkafka)
inline void
KafkaProducer::deliveryCallback(rd_kafka_t* rk, const rd_kafka_message_t* rkmsg, void* /*opaque*/)
{
    if (auto* deliveryCbOpaque = static_cast<DeliveryCbOpaque*>(rkmsg->_private))
    {
        (*deliveryCbOpaque)(rk, rkmsg);
        delete deliveryCbOpaque;
    }
}

inline rd_kafka_resp_err_t
KafkaProducer::sendMessage(const ProducerRecord&             record,
                           std::unique_ptr<DeliveryCbOpaque> opaque,
                           SendOption                        option,
                           ActionWhileQueueIsFull            action)
{
    const auto* topic     = record.topic().c_str();
    const auto  partition = record.partition();
    const auto  msgFlags  = (static_cast<unsigned int>(option == SendOption::ToCopyRecordValue ? RD_KAFKA_MSG_F_COPY : 0)
                             | static_cast<unsigned int>(action == ActionWhileQueueIsFull::Block ? RD_KAFKA_MSG_F_BLOCK : 0));
    const auto* keyPtr    = record.key().data();
    const auto  keyLen    = record.key().size();
    const auto* valuePtr  = record.value().data();
    const auto  valueLen  = record.value().size();

    auto* rk        = getClientHandle();
    auto* opaquePtr = opaque.get();

    rd_kafka_resp_err_t sendResult = RD_KAFKA_RESP_ERR_NO_ERROR;

    if (auto cntHeaders = record.headers().size())
    {
        rd_kafka_headers_t* hdrs = rd_kafka_headers_new(cntHeaders);
        for (const auto& header: record.headers())
        {
            rd_kafka_header_add(hdrs, header.key.c_str(), header.key.size(), header.value.data(), header.value.size());
        }

        sendResult = rd_kafka_producev(rk,
                                       RD_KAFKA_V_TOPIC(topic),
                                       RD_KAFKA_V_PARTITION(partition),
                                       RD_KAFKA_V_MSGFLAGS(msgFlags),
                                       RD_KAFKA_V_HEADERS(hdrs),
                                       RD_KAFKA_V_VALUE(const_cast<void*>(valuePtr), valueLen), // NOLINT
                                       RD_KAFKA_V_KEY(keyPtr, keyLen),                          // NOLINT
                                       RD_KAFKA_V_OPAQUE(opaquePtr),
                                       RD_KAFKA_V_END);
        if (sendResult != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            rd_kafka_headers_destroy(hdrs);
        }
    }
    else
    {
        sendResult = rd_kafka_producev(rk,
                                       RD_KAFKA_V_TOPIC(topic),
                                       RD_KAFKA_V_PARTITION(partition),
                                       RD_KAFKA_V_MSGFLAGS(msgFlags),
                                       RD_KAFKA_V_VALUE(const_cast<void*>(valuePtr), valueLen), // NOLINT
                                       RD_KAFKA_V_KEY(keyPtr, keyLen),                          // NOLINT
                                       RD_KAFKA_V_OPAQUE(opaquePtr),
                                       RD_KAFKA_V_END);
    }

    if (sendResult == RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        // KafkaProducer::deliveryCallback would delete the "opaque"
        opaque.release();
    }

    return sendResult; // NOLINT: leak of memory pointed to by 'opaquePtr' [clang-analyzer-cplusplus.NewDeleteLeaks]
}

inline std::error_code
KafkaProducer::flush(std::chrono::milliseconds timeout)
{
    return ErrorCode(rd_kafka_flush(getClientHandle(), convertMsDurationToInt(timeout)));
}

inline std::error_code
KafkaProducer::close(std::chrono::milliseconds timeout)
{
    _opened = false;

    std::error_code ec = flush(timeout);

    std::string errMsg = ec.message();
    KAFKA_API_DO_LOG(Log::Level::Info, "closed [%s]", errMsg.c_str());

    return ec;
}

inline void
KafkaProducer::initTransactions(std::chrono::milliseconds timeout)
{
    auto error =  Error{rd_kafka_init_transactions(getClientHandle(), static_cast<int>(timeout.count()))};  // NOLINT
    KAFKA_THROW_IF_WITH_ERROR(error);
}

inline void
KafkaProducer::beginTransaction()
{
    auto error = Error{rd_kafka_begin_transaction(getClientHandle())};
    KAFKA_THROW_IF_WITH_ERROR(error);
}

inline void
KafkaProducer::commitTransaction(std::chrono::milliseconds timeout)
{
    auto error = Error{rd_kafka_commit_transaction(getClientHandle(), static_cast<int>(timeout.count()))};  // NOLINT
    KAFKA_THROW_IF_WITH_ERROR(error);
}

inline void
KafkaProducer::abortTransaction(std::chrono::milliseconds timeout)
{
    auto error = Error{rd_kafka_abort_transaction(getClientHandle(), static_cast<int>(timeout.count()))};   // NOLINT
    KAFKA_THROW_IF_WITH_ERROR(error);
}

inline void
KafkaProducer::sendOffsetsToTransaction(const TopicPartitionOffsets& tpos,
                                        const Consumer::ConsumerGroupMetadata& groupMetadata,
                                        std::chrono::milliseconds timeout)
{
    auto rk_tpos = rd_kafka_topic_partition_list_unique_ptr(createRkTopicPartitionList(tpos));
    auto error = Error{rd_kafka_send_offsets_to_transaction(getClientHandle(),
                                                            rk_tpos.get(),
                                                            groupMetadata.rawHandle(),
                                                            static_cast<int>(timeout.count()))};            // NOLINT
    KAFKA_THROW_IF_WITH_ERROR(error);
}

/**
 * A Kafka client that publishes records to the Kafka cluster asynchronously.
 */
class KafkaAsyncProducer: public KafkaProducer
{
public:
    /**
     * The constructor for KafkaAsyncProducer.
     *
     * Options:
     *   - EventsPollingOption::Auto (default) : An internal thread would be started for MessageDelivery callbacks handling.
     *   - EventsPollingOption::Manual         : User have to call the member function `pollEvents()` to trigger MessageDelivery callbacks.
     *
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG      : Invalid BOOTSTRAP_SERVERS property
     *   - RD_KAFKA_RESP_ERR__CRIT_SYS_RESOURCE: Fail to create internal threads
     */
    explicit KafkaAsyncProducer(const Properties&   properties,
                                EventsPollingOption pollOption = EventsPollingOption::Auto)
        : KafkaProducer(KafkaProducer::validateAndReformProperties(properties))
    {
        _pollable = std::make_unique<KafkaClient::PollableCallback<KafkaAsyncProducer>>(this, pollCallbacks);
        if (pollOption == EventsPollingOption::Auto)
        {
            _pollThread = std::make_unique<PollThread>(*_pollable);
        }
    }

    ~KafkaAsyncProducer() override { if (_opened) close(); }

    /**
     * Close this producer. This method waits up to timeout for the producer to complete the sending of all incomplete requests.
     */
    std::error_code close(std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
    {
        _pollThread.reset(); // Join the polling thread (in case it's running)
        _pollable.reset();

        return KafkaProducer::close(timeout);
    }

    /**
     * Asynchronously send a record to a topic.
     *
     * Note:
     *   - If a callback is provided, it's guaranteed to be triggered (before closing the producer).
     *   - If any error occured, an exception would be thrown.
     *   - Make sure the memory block (for ProducerRecord's value) is valid until the delivery callback finishes; Otherwise, should be with option `KafkaProducer::SendOption::ToCopyRecordValue`.
     *
     * Possible errors:
     *   Local errors,
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC:     The topic doesn't exist
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: The partition doesn't exist
     *     - RD_KAFKA_RESP_ERR__INVALID_ARG:       Invalid topic(topic is null, or the length is too long (> 512)
     *     - RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:     No ack received within the time limit
     *     - RD_KAFKA_RESP_ERR__QUEUE_FULL:        The message buffing queue is full
     *   Broker errors,
     *     - [Error Codes] (https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ErrorCodes)
     */
    void send(const ProducerRecord& record, const Producer::Callback& cb, SendOption option = SendOption::NoCopyRecordValue)
    {
        rd_kafka_resp_err_t respErr = sendMessage(record,
                                                  std::make_unique<DeliveryCbOpaque>(record.id(), cb),
                                                  option,
                                                  _pollThread ? ActionWhileQueueIsFull::Block : ActionWhileQueueIsFull::NoBlock);
        KAFKA_THROW_IF_WITH_RESP_ERROR(respErr);
    }

    /**
     * Asynchronously send a record to a topic.
     *
     * Note:
     *   - If a callback is provided, it's guaranteed to be triggered (before closing the producer).
     *   - The input reference parameter `error` will be set if an error occurred.
     *   - Make sure the memory block (for ProducerRecord's value) is valid until the delivery callback finishes; Otherwise, should be with option `KafkaProducer::SendOption::ToCopyRecordValue`.
     *
     * Possible errors:
     *   Local errors,
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC:     The topic doesn't exist
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: The partition doesn't exist
     *     - RD_KAFKA_RESP_ERR__INVALID_ARG:       Invalid topic(topic is null, or the length is too long (> 512)
     *     - RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:     No ack received within the time limit
     *     - RD_KAFKA_RESP_ERR__QUEUE_FULL:        The message buffing queue is full
     *   Broker errors,
     *     - [Error Codes] (https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ErrorCodes)
     */
    void send(const ProducerRecord& record, const Producer::Callback& cb, std::error_code& ec, SendOption option = SendOption::NoCopyRecordValue)
    {
        rd_kafka_resp_err_t respErr = sendMessage(record,
                                                  std::make_unique<DeliveryCbOpaque>(record.id(), cb),
                                                  option,
                                                  _pollThread ? ActionWhileQueueIsFull::Block : ActionWhileQueueIsFull::NoBlock);
        ec = ErrorCode(respErr);
    }

    /**
     * Call the MessageDelivery callbacks (if any)
     * Note: The KafkaAsyncProducer MUST be constructed with option `EventsPollingOption::Manual`.
     */
    void pollEvents(std::chrono::milliseconds timeout)
    {
        assert(!_pollThread);

        _pollable->poll(convertMsDurationToInt(timeout));
    }

private:
    std::unique_ptr<Pollable>   _pollable;
    std::unique_ptr<PollThread> _pollThread;

    static void pollCallbacks(KafkaAsyncProducer* producer, int timeoutMs)
    {
        rd_kafka_poll(producer->getClientHandle(), timeoutMs);
    }
};

/**
 * A Kafka client that publishes records to the Kafka cluster asynchronously.
 */
class KafkaSyncProducer: public KafkaProducer
{
public:
    /**
     * The constructor for KafkaSyncProducer.
     * Throws KafkaException with errors:
     *   - RD_KAFKA_RESP_ERR__INVALID_ARG:       Invalid BOOTSTRAP_SERVERS property
     *   - RD_KAFKA_RESP_ERR__CRIT_SYS_RESOURCE: Fail to create internal threads
     */
    explicit KafkaSyncProducer(const Properties& properties)
        : KafkaProducer(KafkaSyncProducer::validateAndReformProperties(properties))
    {
    }

    ~KafkaSyncProducer() override { if (_opened) close(); }

    /**
     * Synchronously send a record to a topic.
     * Throws KafkaException with errors:
     *   Local errors,
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC:     The topic doesn't exist
     *     - RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION: The partition doesn't exist
     *     - RD_KAFKA_RESP_ERR__INVALID_ARG:       Invalid topic(topic is null, or the length is too long (> 512)
     *     - RD_KAFKA_RESP_ERR__MSG_TIMED_OUT:     No ack received within the time limit
     *   Broker errors,
     *     - [Error Codes] (https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ErrorCodes)
     */
    Producer::RecordMetadata send(const ProducerRecord& record)
    {
        std::error_code deliveryError;
        std::promise<Producer::RecordMetadata> metadataPromise;
        auto metadataFuture = metadataPromise.get_future();

        auto cb = [&deliveryError, &metadataPromise] (const Producer::RecordMetadata& metadata, std::error_code ec) {
            deliveryError = ec;
            metadataPromise.set_value(metadata);
        };
        rd_kafka_resp_err_t sendError = sendMessage(record,
                                                    std::make_unique<DeliveryCbOpaque>(record.id(), cb),
                                                    SendOption::NoCopyRecordValue,
                                                    ActionWhileQueueIsFull::NoBlock);
        KAFKA_THROW_IF_WITH_RESP_ERROR(sendError);

        while (metadataFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        {
            rd_kafka_poll(getClientHandle(), CALLBACK_POLLING_INTERVAL_MS);
        }

        auto metadata = metadataFuture.get();
        KAFKA_THROW_IF_WITH_RESP_ERROR(static_cast<rd_kafka_resp_err_t>(deliveryError.value()));

        return metadata;
    }

    /**
     * Close this producer. This method waits up to timeout for the producer to complete the sending of all incomplete requests.
     */
    std::error_code close(std::chrono::milliseconds timeout = std::chrono::milliseconds::max())
    {
        return KafkaProducer::close(timeout);
    }
private:
    static Properties validateAndReformProperties(const Properties& origProperties)
    {
        // Let the base class validate first
        Properties properties = KafkaProducer::validateAndReformProperties(origProperties);

        // KafkaSyncProducer sends only one message each time, -- no need to wait for batching
        properties.put(ProducerConfig::LINGER_MS, "0");

        return properties;
    }
};

} // end of KAFKA_API

