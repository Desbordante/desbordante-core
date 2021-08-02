#pragma once

#include "kafka/Project.h"

#include "kafka/Types.h"


namespace KAFKA_API {

/**
 * Configuration for the Kafka Producer.
 */
class ProducerConfig: public Properties
{
public:
    ProducerConfig() = default;
    ProducerConfig(const ProducerConfig&) = default;
    explicit ProducerConfig(const PropertiesMap& kvMap): Properties(kvMap) {}

    /**
     * The string contains host:port pairs of brokers (splitted by ",") that the producer will use to establish initial connection to the Kafka cluster.
     * Note: It's mandatory.
     */
    static const constexpr char* BOOTSTRAP_SERVERS            = "bootstrap.servers";

    /**
     * This can be any string, and will be used by the brokers to identify messages sent from the client.
     */
    static const constexpr char* CLIENT_ID                    = "client.id";

    /**
     * The acks parameter controls how many partition replicas must receive the record before the producer can consider the write successful.
     *    1) acks=0, the producer will not wait for a reply from the broker before assuming the message was sent successfully.
     *    2) acks=1, the producer will receive a success response from the broker the moment the leader replica received the message.
     *    3) acks=all, the producer will receive a success response from the broker once all in-sync replicas received the message.
     * Note: if "ack=all", please make sure the topic's replication factor be larger than 1.
     *      That means, if the topic is automaticly created by producer's `send`, the `default.replication.factor` property for the kafka server should be larger than 1.
     *      The "ack=all" property is mandatory for reliability requirements, but would increase the ack latency and impact the throughput.
     * Default value: all
     */
    static const constexpr char* ACKS                         = "acks";

    /**
     * Maximum number of messages allowed on the producer queue.
     * Default value: 100000
     */
    static const constexpr char* QUEUE_BUFFERING_MAX_MESSAGES = "queue.buffering.max.messages";

    /**
     * Maximum total message size sum allowed on the producer queue.
     * Default value: 0x100000 (1GB)
     */
    static const constexpr char* QUEUE_BUFFERING_MAX_KBYTES   = "queue.buffering.max.kbytes";

    /**
     * Delay in milliseconds to wait for messages in the producer queue, to accumulate before constructing messages batches to transmit to brokers.
     * Default value: 0 (KafkaSyncProducer); 0.5 (KafkaAsyncProducer)
     */
    static const constexpr char* LINGER_MS                    = "linger.ms";

    /**
     * Maximum number of messages batched in one messageSet. The total MessageSet size is also limited by MESSAGE_MAX_BYTES.
     * Default value: 10000
     */
    static const constexpr char* BATCH_NUM_MESSAGES           = "batch.num.messages";

    /**
     * Maximum size (in bytes) of all messages batched in one MessageSet (including protocol framing overhead).
     * Default value: 1000000
     */
    static const constexpr char* BATCH_SIZE                   = "batch.size";

    /**
     * Maximum Kafka protocol request message size.
     * Note: Should be coordinated with the bokers's configuration. Otherwise, any larger message would be rejected!
     * Default value: 1000000
     */
    static const constexpr char* MESSAGE_MAX_BYTES            = "message.max.bytes";

    /**
     * This value is enforced locally and limits the time a produced message waits for successful delivery.
     * Note: If failed to get the ack within this limit, an exception would be thrown (in `SyncProducer.send()`), or an error code would be passed into the delivery callback (AsyncProducer).
     * Default value: 300000
     */
    static const constexpr char* MESSAGE_TIMEOUT_MS           = "message.timeout.ms";

    /**
     * This value is only enforced by the brokers and relies on `ACKS` being non-zero.
     * Note: The leading broker waits for in-sync replicas to acknowledge the message, and will return an error if the time elapses without the necessary acks.
     * Default value: 5000
     */
    static const constexpr char* REQUEST_TIMEOUT_MS           = "request.timeout.ms";

    /**
     * The default partitioner for a ProducerRecord (with no partition assigned).
     * Note: It's not the same with Java version's "partitioner.class" property
     * Available options:
     *     1) random            -- random distribution
     *     2) consistent        -- CRC32 hash of key (`ProducerRecord`s with empty key are mapped to single partition)
     *     3) consistent_random -- CRC32 hash of key (`ProducerRecord`s with empty key are randomly partitioned)
     *     4) murmur2           -- Java Producer compatible Murmur2 hash of key (`ProducerRecord`s with empty key are mapped to single partition)
     *     5) murmur2_random    -- Java Producer compatible Murmur2 hash of key (`ProducerRecord`s with empty key are randomly partitioned. It's equivalent to the Java Producer's default partitioner)
     *     6) fnv1a             -- FNV-1a hash of key (`ProducerRecord`s with empty key are mapped to single partition)
     *     7) fnv1a_random      -- FNV-1a hash of key (`ProducerRecord`s with empty key are randomly partitioned)
     * Default value: murmur2_random
     */
    static const constexpr char* PARTITIONER                  = "partitioner";

    /**
     * Maximum number of in-flight requests per broker connection.
     * Default value: 1000000 (while `enable.idempotence`=false); 5 (while `enable.idempotence`=true)
     */
    static const constexpr char* MAX_IN_FLIGHT                = "max.in.flight";

    /**
     * When set to `true`, the producer will ensure that messages are succefully sent exactly once and in the original order.
     * Default value: false
     */
    static const constexpr char* ENABLE_IDEMPOTENCE           = "enable.idempotence";

    /**
     * It's used to identify the same transactional producer instance across process restarts.
     */
    static const constexpr char* TRANSACTIONAL_ID             = "transactional.id";

    /**
     * Th maximus amount of time in milliseconds that the transaction coordinator will wait for a trnsaction status update from the producer before proactively ablrting the ongoing transaction.
     * Default value: 60000
     */
    static const constexpr char* TRANSACTION_TIMEOUT_MS       = "transaction.timeout.ms";

    /**
     * Protocol used to communicate with brokers.
     * Default value: plaintext
     */
    static const constexpr char* SECURITY_PROTOCOL            = "security.protocol";

    /**
     * Shell command to refresh or acquire the client's Kerberos ticket.
     */
    static const constexpr char* SASL_KERBEROS_KINIT_CMD      = "sasl.kerberos.kinit.cmd";

    /**
     * The client's Kerberos principal name.
     */
    static const constexpr char* SASL_KERBEROS_SERVICE_NAME   = "sasl.kerberos.service.name";
};

}

