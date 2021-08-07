#include "kafka/KafkaConsumer.h"
#include <string>

class kafkaConsumerManager{
    kafka::Properties props;
    kafka::KafkaManualCommitConsumer consumer;

public:

    kafkaConsumerManager() = delete;

    kafkaConsumerManager(std::string brokers) 
        : props({{ "bootstrap.servers", brokers}, {"max.poll.records", "1"} }), consumer(props) { }

    kafka::KafkaManualCommitConsumer& getConsumer() {
        return consumer;
    }

};