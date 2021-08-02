#pragma once

#include "kafka/Project.h"

#include "kafka/Error.h"
#include "kafka/Header.h"
#include "kafka/Timestamp.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <sstream>


namespace KAFKA_API {

/**
 * A key/value pair to be received from Kafka.
 * This also consists of a topic name and a partition number from which the record is being received, an offset that points to the record in a Kafka partition
 */
class ConsumerRecord
{
public:
    // ConsumerRecord will take the ownership of msg (rd_kafka_message_t*)
    explicit ConsumerRecord(rd_kafka_message_t* msg): _rk_msg(msg, rd_kafka_message_destroy) {}

    /**
     * The topic this record is received from.
     */
    Topic       topic()         const { return _rk_msg->rkt ? rd_kafka_topic_name(_rk_msg->rkt): ""; }

    /**
     * The partition from which this record is received.
     */
    Partition   partition()     const { return _rk_msg->partition; }

    /**
     * The position of this record in the corresponding Kafka partition.
     */
    Offset      offset()        const { return _rk_msg->offset; }

    /**
     * The key (or null if no key is specified).
     */
    Key         key()           const { return Key(_rk_msg->key, _rk_msg->key_len); }

    /**
     * The value.
     */
    Value       value()         const { return Value(_rk_msg->payload, _rk_msg->len); }

    /**
     * The timestamp of the record.
     */
    Timestamp   timestamp() const
    {
        rd_kafka_timestamp_type_t tstype{};
        Timestamp::Value tsValue = rd_kafka_message_timestamp(_rk_msg.get(), &tstype);
        return {tsValue, tstype};
    }

    /**
     * The headers of the record.
     */
    Headers headers() const;

    /**
     * Return just one (the very last) header's value for the given key.
     */
    Header::Value lastHeaderValue(const Header::Key& key);

    /**
     * The error.
     *
     * Possible cases:
     *   1. Success
     *     - RD_KAFKA_RESP_ERR_NO_ERROR (0),     -- got a message successfully
     *     - RD_KAFKA_RESP_ERR__PARTITION_EOF,   -- reached the end of a partition (got no message)
     *   2. Failure
     *     - [Error Codes] (https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ErrorCodes)
     */
    std::error_code error() const { return ErrorCode(_rk_msg->err); }

    /**
    * Obtains explanatory string.
    */
    std::string toString() const;

private:
    using rd_kafka_message_unique_ptr = std::unique_ptr<rd_kafka_message_t, void(*)(rd_kafka_message_t*)>;
    rd_kafka_message_unique_ptr _rk_msg;
};

inline Headers
ConsumerRecord::headers() const
{
    Headers headers;

    rd_kafka_headers_t* hdrs = nullptr;
    if (rd_kafka_message_headers(_rk_msg.get(), &hdrs) != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        return headers;
    }

    headers.reserve(rd_kafka_header_cnt(hdrs));

    const char* name      = nullptr;
    const void* valuePtr  = nullptr;
    std::size_t valueSize = 0;
    for (int i = 0; !rd_kafka_header_get_all(hdrs, i, &name, &valuePtr, &valueSize); i++)
    {
        headers.emplace_back(name, Header::Value(valuePtr, valueSize));
    }

    return headers;
}

inline Header::Value
ConsumerRecord::lastHeaderValue(const Header::Key& key)
{
    rd_kafka_headers_t* hdrs = nullptr;
    if (rd_kafka_message_headers(_rk_msg.get(), &hdrs) != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        return Header::Value();
    }

    const void* valuePtr  = nullptr;
    std::size_t valueSize = 0;
    return (rd_kafka_header_get_last(hdrs, key.c_str(), &valuePtr, &valueSize) == RD_KAFKA_RESP_ERR_NO_ERROR) ?
           Header::Value(valuePtr, valueSize) : Header::Value();
}

inline std::string
ConsumerRecord::toString() const
{
    std::ostringstream oss;
    if (!error())
    {
        oss << topic() << "-" << partition() << ":" << offset() << ", " << timestamp().toString() << ", "
            << (key().size() ? (key().toString() + "/") : "") << value().toString();
    }
    else if (error().value() == RD_KAFKA_RESP_ERR__PARTITION_EOF)
    {
        oss << "EOF[" << topic() << "-" << partition() << ":" << offset() << "]";
    }
    else
    {
        oss << "ERROR[" << error().message() << ", " << topic() << "-" << partition() << ":" << offset() << "]";
    }
    return oss.str();
}

}

