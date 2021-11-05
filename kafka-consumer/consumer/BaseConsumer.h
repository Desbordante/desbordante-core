#pragma once
#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"

#include "../db/DBManager.h"

using cppkafka::Consumer;
using cppkafka::Message;
using cppkafka::TopicPartitionList;

class BaseConsumer{
private:
    virtual void processMsg(nlohmann::json payload, 
                            DBManager const &manager) const = 0;
protected:
    std::unique_ptr<cppkafka::Consumer> consumer;
    std::string const topicName;
public:
    explicit BaseConsumer(std::string brokers, std::string groupID, 
                          std::string topicName) 
        : topicName(topicName)
        {
            cppkafka::Configuration config = 
            {
                { "metadata.broker.list", brokers },
                { "group.id", groupID },
                { "enable.auto.commit", false }
            };
            consumer = std::make_unique<cppkafka::Consumer>(config);
            consumer->subscribe({ topicName });
        }

    virtual ~BaseConsumer() = default;

    auto getTopicName() const {
        return topicName;
    }
    
    void run(DBManager const &manager) const {
    try {
        std::cout << "% Reading messages from topic: " 
                  << topicName << std::endl;
        while (true) {
            auto msg = consumer->poll();
            if (msg) {
                if (msg.get_error()) {
                    if (!msg.is_eof()) {
                        std::cout << "[+] Received error notification: " 
                                  << msg.get_error() << std::endl;
                    }
                } else {
                    if (msg.get_key()) {
                        std::cout << msg.get_key() << std::endl;
                    }
                    auto payload = nlohmann::json::parse(msg.get_payload());
                    processMsg(payload, manager);
                    consumer->commit(msg);
                }
            }
        }
    } catch (const cppkafka::ConsumerException& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
        throw e;
    }
}
};