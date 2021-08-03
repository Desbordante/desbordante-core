#include "kafka/KafkaConsumer.h"
#include <string>

class kafkaConsumerManager{
    kafka::Properties props;
    kafka::KafkaManualCommitConsumer consumer;

public:

    kafkaConsumerManager() = delete;

    kafkaConsumerManager(std::string brokers) 
        : props({{ "bootstrap.servers", brokers}}), consumer(props) { }

    kafka::KafkaManualCommitConsumer& getConsumer() {
        return consumer;
    }

};