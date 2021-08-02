#pragma once

#include "kafka/Project.h"

#include "kafka/Header.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"


namespace KAFKA_API {

/**
 * A key/value pair to be sent to Kafka.
 * This consists of a topic name to which the record is being sent, an optional partition number, and an optional key and value.
 */
class ProducerRecord
{
public:
    using Id  = std::uint64_t;

    // Note: ProducerRecord would not take the ownership from these parameters,
    ProducerRecord(Topic topic, Partition partition, const Key& key, const Value& value, Id id = 0)
       : _topic(std::move(topic)), _partition(partition), _key(key), _value(value), _id(id) {}
    ProducerRecord(const Topic& topic, const Key& key, const Value& value, Id id = 0)
        : ProducerRecord(topic, RD_KAFKA_PARTITION_UA, key, value, id) {}

    /**
     * The topic this record is being sent to.
     */
    const Topic& topic()  const { return _topic; }

    /**
     * The partition to which the record will be sent (or UNKNOWN_PARTITION if no partition was specified).
     */
    Partition partition() const { return _partition; }

    /**
     * The key (or null if no key is specified).
     */
    Key       key()       const { return _key; }

    /**
     * The value.
     */
    Value     value()     const { return _value; }

    /**
     * The id to identify the message (consistent with `Producer::Metadata::recordId()`).
     */
    Id        id()        const { return _id; }

    /**
     * The headers.
     */
    const Headers& headers() const { return _headers; }

    /**
     * The headers.
     * Note: Users could set headers with the reference.
     */
    Headers&       headers()       { return _headers; }

    /**
     * Set the partition.
     */
    void setPartition(Partition partition) { _partition = partition; }

    /**
     * Set the key.
     */
    void setKey(const Key& key)            { _key = key; }

    /**
     * Set the value.
     */
    void setValue(const Value& value)      { _value = value; }

    /**
     * Set the record id.
     */
    void setId(Id id)                      { _id = id; }

    std::string toString() const
    {
        return _topic + "-" + (_partition == RD_KAFKA_PARTITION_UA ? "NA" : std::to_string(_partition)) + std::string(":") + std::to_string(_id)
            + std::string(", ") + (_headers.empty() ? "" : ("headers[" + KAFKA_API::toString(_headers) + "], "))
            + _key.toString() + std::string("/") + _value.toString();
    }

private:
    Topic     _topic;
    Partition _partition;
    Key       _key;
    Value     _value;
    Id        _id;
    Headers   _headers;
};

}

