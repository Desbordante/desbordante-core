#include <string>
#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"

using cppkafka::Consumer;
using cppkafka::Message;
using cppkafka::TopicPartitionList;

class ConsumerManager{
    cppkafka::Configuration config;  
    std::unique_ptr<cppkafka::Consumer> consumer;

public:

    explicit ConsumerManager(std::string brokers, std::string group_id) 
        {
            config = {
                { "metadata.broker.list", brokers },
                { "group.id", group_id },
                { "enable.auto.commit", false }
            };
            consumer = std::make_unique<cppkafka::Consumer>(config);
        }

    auto getConsumer() {
        return consumer.get();
    }

};