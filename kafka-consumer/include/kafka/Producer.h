#pragma once

#include "kafka/Project.h"

#include "kafka/ProducerRecord.h"
#include "kafka/RdKafkaHelper.h"
#include "kafka/Timestamp.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <functional>
#include <memory>


namespace KAFKA_API {

/**
 * Namespace for datatypes defined for KafkaProducer.
 */
namespace Producer
{
    /**
     * The metadata for a record that has been acknowledged by the server.
     */
    class RecordMetadata
    {
    public:
        enum class PersistedStatus { Not, Possibly, Done };

        RecordMetadata() = default;

        RecordMetadata(const RecordMetadata& another) { *this = another; }

        // This is only called by the KafkaProducer::deliveryCallback (with a valid rkmsg pointer)
        RecordMetadata(const rd_kafka_message_t* rkmsg, ProducerRecord::Id recordId)
            : _cachedInfo(), _rkmsg(rkmsg), _recordId(recordId) {}

        RecordMetadata& operator=(const RecordMetadata& another)
        {
            if (this != &another)
            {
                _cachedInfo = std::make_unique<CachedInfo>(another.topic(),
                                                           another.partition(),
                                                           another.offset() ? *another.offset() : RD_KAFKA_OFFSET_INVALID,
                                                           another.keySize(),
                                                           another.valueSize(),
                                                           another.timestamp(),
                                                           another.persistedStatus());
                _recordId = another._recordId;
                _rkmsg    = nullptr;
            }

            return *this;
        }

        /**
         * The topic the record was appended to.
         */
        std::string           topic()      const
        {
            return _rkmsg ? (_rkmsg->rkt ? rd_kafka_topic_name(_rkmsg->rkt) : "") : _cachedInfo->topic;
        }

        /**
         * The partition the record was sent to.
         */
        Partition             partition()  const
        {
            return _rkmsg ? _rkmsg->partition : _cachedInfo->partition;
        }

        /**
         * The offset of the record in the topic/partition.
         */
        Optional<Offset>      offset()     const
        {
            auto offset = _rkmsg ? _rkmsg->offset : _cachedInfo->offset;
            return (offset != RD_KAFKA_OFFSET_INVALID) ? Optional<Offset>(offset) : Optional<Offset>();
        }

        /**
         * The recordId could be used to identify the acknowledged message.
         */
        ProducerRecord::Id    recordId()   const
        {
            return _recordId;
        }

        /**
         * The size of the key in bytes.
         */
        KeySize               keySize()    const
        {
            return _rkmsg ? _rkmsg->key_len : _cachedInfo->keySize;
        }

        /**
         * The size of the value in bytes.
         */
        ValueSize             valueSize()  const
        {
            return _rkmsg ? _rkmsg->len : _cachedInfo->valueSize;
        }

        /**
         * The timestamp of the record in the topic/partition.
         */
        Timestamp             timestamp()  const
        {
            return _rkmsg ? getMsgTimestamp(_rkmsg) : _cachedInfo->timestamp;
        }

        /**
         * The persisted status of the record.
         */
        PersistedStatus       persistedStatus()  const
        {
            return _rkmsg ? getMsgPersistedStatus(_rkmsg) : _cachedInfo->persistedStatus;
        }

        std::string           persistedStatusString() const
        {
            return getPersistedStatusString(persistedStatus());
        }

        std::string toString() const
        {
            return topic() + "-" + std::to_string(partition()) + "@" + (offset() ? std::to_string(*offset()) : "NA")
                   + ":id[" + std::to_string(recordId()) + "]," + timestamp().toString() + "," + persistedStatusString();
        }

    private:
        static Timestamp getMsgTimestamp(const rd_kafka_message_t* rkmsg)
        {
            rd_kafka_timestamp_type_t tstype{};
            Timestamp::Value tsValue = rd_kafka_message_timestamp(rkmsg, &tstype);
            return {tsValue, tstype};
        }

        static PersistedStatus getMsgPersistedStatus(const rd_kafka_message_t* rkmsg)
        {
            rd_kafka_msg_status_t status = rd_kafka_message_status(rkmsg);
            return status == RD_KAFKA_MSG_STATUS_NOT_PERSISTED ? PersistedStatus::Not : (status == RD_KAFKA_MSG_STATUS_PERSISTED ? PersistedStatus::Done : PersistedStatus::Possibly);
        }

        static std::string getPersistedStatusString(PersistedStatus status)
        {
            return status == PersistedStatus::Not ? "NotPersisted" :
                (status == PersistedStatus::Done ? "Persisted" : "PossiblyPersisted");
        }

        struct CachedInfo
        {
            CachedInfo(Topic t, Partition p, Offset o, KeySize ks, ValueSize vs, Timestamp ts, PersistedStatus pst)
                : topic(std::move(t)),
                  partition(p),
                  offset(o),
                  keySize(ks),
                  valueSize(vs),
                  timestamp(ts),
                  persistedStatus(pst)
            {
            }

            CachedInfo(const CachedInfo&) = default;

            std::string     topic;
            Partition       partition;
            Offset          offset;
            KeySize         keySize;
            ValueSize       valueSize;
            Timestamp       timestamp;
            PersistedStatus persistedStatus;
        };

        std::unique_ptr<CachedInfo> _cachedInfo;
        const rd_kafka_message_t*   _rkmsg    = nullptr;
        ProducerRecord::Id          _recordId = 0;
    };

    /**
     * A callback method could be used to provide asynchronous handling of request completion.
     * This method will be called when the record sent (by KafkaAsyncProducer) to the server has been acknowledged.
     */
    using Callback = std::function<void(const RecordMetadata& metadata, std::error_code ec)>;

} // end of Producer


} // end of KAFKA_API

