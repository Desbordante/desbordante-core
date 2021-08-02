#pragma once

#include "kafka/Project.h"

#include "kafka/KafkaException.h"
#include "kafka/Types.h"

#include "librdkafka/rdkafka.h"

#include <map>
#include <vector>


namespace KAFKA_API {

/**
 * The metadata info for a topic.
 */
struct BrokerMetadata {
    /**
     * Information for a Kafka node.
     */
    struct Node
    {
      public:
        using Id   = int;
        using Host = std::string;
        using Port = int;

        Node(Id i, Host h, Port p): id(i), host(std::move(h)), port(p) {}

        /**
         * The node id.
         */
        Node::Id  id;

        /**
         * The host name.
         */
        Node::Host host;

        /**
         * The port.
         */
        Node::Port port;

        /**
         * Obtains explanatory string.
         */
        std::string toString() const { return host + ":" + std::to_string(port) + "/" + std::to_string(id); }
    };

    /**
     * It is used to describe per-partition state in the MetadataResponse.
     */
    struct PartitionInfo
    {
        explicit PartitionInfo(Node::Id leaderId): leader(leaderId) {}

        void addReplica(Node::Id id)       { replicas.emplace_back(id); }
        void addInSyncReplica(Node::Id id) { inSyncReplicas.emplace_back(id); }

        /**
         * The node id currently acting as a leader for this partition or null if there is no leader.
         */
        Node::Id              leader;

        /**
         * The complete set of replicas id for this partition regardless of whether they are alive or up-to-date.
         */
        std::vector<Node::Id> replicas;

        /**
         * The subset of the replicas id that are in sync, that is caught-up to the leader and ready to take over as leader if the leader should fail.
         */
        std::vector<Node::Id> inSyncReplicas;

    };

    /**
     * Obtains explanatory string from Node::Id.
     */
    std::string getNodeDescription(Node::Id id) const;

    /**
     * Obtains explanatory string for PartitionInfo.
     */
    std::string toString(const PartitionInfo& partitionInfo) const;

    /**
     * The BrokerMetadata is per-topic constructed.
     */
    explicit BrokerMetadata(Topic topic): _topic(std::move(topic)) {}

    /**
     * The topic name.
     */
    const std::string& topic() const { return _topic; }

    /**
     * The nodes info in the MetadataResponse.
     */
    std::vector<std::shared_ptr<Node>> nodes() const;

    /**
     * The partitions' state in the MetadataResponse.
     */
    const std::map<Partition, PartitionInfo>& partitions() const { return _partitions; }

    /**
     * Obtains explanatory string.
     */
    std::string toString()   const;

    void setOrigNodeName(const std::string& origNodeName)                          { _origNodeName = origNodeName; }
    void addNode(Node::Id nodeId, const Node::Host& host, Node::Port port)         { _nodes[nodeId] = std::make_shared<Node>(nodeId, host, port); }
    void addPartitionInfo(Partition partition, const PartitionInfo& partitionInfo) { _partitions.emplace(partition, partitionInfo); }

private:
    Topic                                     _topic;
    std::string                               _origNodeName;
    std::map<Node::Id, std::shared_ptr<Node>> _nodes;
    std::map<Partition, PartitionInfo>        _partitions;
};

inline std::vector<std::shared_ptr<BrokerMetadata::Node>>
BrokerMetadata::nodes() const
{
    std::vector<std::shared_ptr<BrokerMetadata::Node>> ret;
    for (const auto& nodeInfo: _nodes)
    {
        ret.emplace_back(nodeInfo.second);
    }
    return ret;
}

inline std::string
BrokerMetadata::getNodeDescription(Node::Id id) const
{
    const auto& found = _nodes.find(id);
    if (found == _nodes.cend()) return "-:-/" + std::to_string(id);

    auto node = found->second;
    return node->host + ":" + std::to_string(node->port) + "/" + std::to_string(id);
}

inline std::string
BrokerMetadata::toString(const PartitionInfo& partitionInfo) const
{
    std::ostringstream oss;

    auto streamNodes = [this](std::ostringstream& ss, const std::vector<Node::Id>& nodeIds) -> std::ostringstream& {
        bool isTheFirst = true;
        for (const auto id: nodeIds)
        {
            ss << (isTheFirst ? (isTheFirst = false, "") : ", ") << getNodeDescription(id);
        }
        return ss;
    };

    oss << "leader[" << getNodeDescription(partitionInfo.leader) << "], replicas[";
    streamNodes(oss, partitionInfo.replicas) << "], inSyncReplicas[";
    streamNodes(oss, partitionInfo.inSyncReplicas) << "]";

    return oss.str();
}

inline std::string
BrokerMetadata::toString() const
{
    std::ostringstream oss;

    oss << "originatingNode[" << _origNodeName << "], topic[" << _topic <<  "], partitions{";
    bool isTheFirst = true;
    for (const auto& partitionInfoPair: _partitions)
    {
        const Partition       partition     = partitionInfoPair.first;
        const PartitionInfo&  partitionInfo = partitionInfoPair.second;
        oss << (isTheFirst ? (isTheFirst = false, "") : "; ") << partition << ": " << toString(partitionInfo);
    }
    oss << "}";

    return oss.str();
}

}

