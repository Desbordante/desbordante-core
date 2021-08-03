#include "db/DBManager.h"
#include "consumer/consumerManager.h"
#include "consumer/consumerStart.h"

int main()
{
    try {
        DBManager manager("localhost", "root", "5432", "desbordante", "postgres");
        // Create a consumerManager instance by passing configuration object.
        kafkaConsumerManager consumerManager("localhost:9092");
        // Get consumer instance from object consumerManager
        kafka::KafkaManualCommitConsumer& consumer = consumerManager.getConsumer();
        // Start consumer for topic 'tasks'
        tasksConsumerStart(manager, consumer, "tasks");
    } catch (const std::exception& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
    }
}