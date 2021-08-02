#pragma once

#include "kafka/Project.h"

#include "kafka/Properties.h"


namespace KAFKA_API {

/**
 * Configuration for the Kafka Consumer.
 */
class AdminClientConfig: public Properties
{
public:
    AdminClientConfig() = default;
    AdminClientConfig(const AdminClientConfig&) = default;
    explicit AdminClientConfig(const PropertiesMap& kvMap): Properties(kvMap) {}

    /**
     * The string contains host:port pairs of brokers (splitted by ",") that the administrative client will use to establish initial connection to the Kafka cluster.
     * Note: It's mandatory.
     */
    static const constexpr char* BOOTSTRAP_SERVERS          = "bootstrap.servers";

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

