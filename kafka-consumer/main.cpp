#include "src/db/DBManager.h"
#include "src/consumerManager.h"
#include "src/consumerStart.h"

bool checkOptions(std::string const& alg, double error) {
    if (alg != "pyro" && alg != "tane") {
        std::cout << "ERROR: no matching algorithm. Available algorithms are:\n\tpyro\n\ttane.\n" << std::endl;
        return false;
    }
    if (error > 1 || error < 0) {
        std::cout << "ERROR: error should be between 0 and 1.\n" << std::endl;
    }
    return true;
}

int main()
{
    DBManager manager("localhost", "root", "5432", "desbordante", "postgres");

    try {
        // Create a consumerManager instance by passing configuration object.
        kafkaConsumerManager consumerManager("localhost:9092");
        // Get consumer instance from object consumerManager
        kafka::KafkaManualCommitConsumer& consumer = consumerManager.getConsumer();
        // Start consumer for topic 'tasks'
        tasksConsumerStart(manager, consumer, "tasks");

    } catch (const kafka::KafkaException& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
    }
}