#pragma once

#include "kafka/Project.h"

#include "kafka/Properties.h"


namespace KAFKA_API {

/**
 * Configuration for the Kafka Consumer.
 */
class ConsumerConfig: public Properties
{
public:
    ConsumerConfig() = default;
    ConsumerConfig(const ConsumerConfig&) = default;
    explicit ConsumerConfig(const PropertiesMap& kvMap): Properties(kvMap) {}

    /**
     * The string contains host:port pairs of brokers (splitted by ",") that the consumer will use to establish initial connection to the Kafka cluster.
     * Note: It's mandatory.
     */
    static const constexpr char* BOOTSTRAP_SERVERS       = "bootstrap.servers";

    /**
     * Group identifier.
     * Note: It's better to configure it manually, otherwise a random one would be used for it.
     *
     */
    static const constexpr char* GROUP_ID                = "group.id";

    /**
     * Client identifier.
     */
    static const constexpr char* CLIENT_ID               = "client.id";

    /**
     * This property controls the behavior of the consumer when it starts reading a partition for which it doesn't have a valid committed offset.
     * The "latest" means the consumer will begin reading the newest records written after the consumer started. While "earliest" means that the consumer will read from the very beginning.
     * Available options: latest, earliest
     * Default value: latest
     */
    static const constexpr char* AUTO_OFFSET_RESET       = "auto.offset.reset";

    /**
     * Emit RD_KAFKA_RESP_ERR_PARTITION_EOF event whenever the consumer reaches the end of a partition.
     * Default value: false
     */
    static const constexpr char* ENABLE_PARTITION_EOF    = "enable.partition.eof";

    /**
     * This controls the maximum number of records that a single call to poll() will return.
     * Default value: 500
     */
    static const constexpr char* MAX_POLL_RECORDS        = "max.poll.records";

    /**
     * Minimum number of messages per topic/partition tries to maintain in the local consumer queue.
     * Note: With a larger value configured, the consumer would send FetchRequest towards brokers more frequently.
     * Defalut value: 100000
     */
    static const constexpr char* QUEUED_MIN_MESSAGES     = "queued.min.messages";

    /**
     * Client group session and failure detection timeout.
     * If no heartbeat received by the broker within this timeout, the broker will remove the consumer and trigger a rebalance.
     * Default value: 10000
     */
    static const constexpr char* SESSION_TIMEOUT_MS      = "session.timeout.ms";

    /**
     * Timeout for network requests.
     * Default value: 60000
     */
    static const constexpr char* SOCKET_TIMEOUT_MS       = "socket.timeout.ms";

    /**
     * Control how to read messages written transactionally.
     * Available options: read_uncommitted, read_committed
     * Default value: read_committed
     */
    static const constexpr char* ISOLATION_LEVEL         = "isolation.level";

    /*
     * The name of one or more partition assignment strategies.
     * The elected group leader will use a strategy supported by all members of the group to assign partitions to group members.
     * Available options: range, roundrobin, cooperative-sticky
     * Default value: range,roundrobin
     */
    static const constexpr char* PARTITION_ASSIGNMENT_STRATEGY = "partition.assignment.strategy";
    /**
     * Protocol used to communicate with brokers.
     * Default value: plaintext
     */
    static const constexpr char* SECURITY_PROTOCOL          = "security.protocol";

    /**
     * Shell command to refresh or acquire the client's Kerberos ticket.
     */
    static const constexpr char* SASL_KERBEROS_KINIT_CMD    = "sasl.kerberos.kinit.cmd";

    /**
     * The client's Kerberos principal name.
     */
    static const constexpr char* SASL_KERBEROS_SERVICE_NAME = "sasl.kerberos.service.name";
};

}

