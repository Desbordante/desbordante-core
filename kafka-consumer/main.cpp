#include "db/DBManager.h"
#include "consumer/consumerManager.h"
#include "consumer/consumerStart.h"
#include "json.hpp"

static std::string pg_connection(std::string const &filename, std::string const &dbname) {
    
    std::ifstream pg_config(filename);
    nlohmann::json j;
    pg_config >> j;

    std::string connection = "postgresql://";
    if(j.find("user") != j.end()){
        std::string user = j["user"];
        if(!user.empty()) connection += user;
    }
    if(j.find("password") != j.end()){
        std::string password = j["password"];
        if(!password.empty()) connection += ":" + password;
    }
    if (j.find("host") != j.end()){
        std::string host = j["host"];
        if (!host.empty()) connection += "@" + host;
    } else {
        connection += "@localhost";
    }
    if (j.find("port") != j.end()){
        std::string port = j["port"];
        connection += ":" + port;
    }
    return connection + "/" + dbname;
}

int main()
{
    try {
        DBManager manager(pg_connection("pg_config.json", "desbordante"));
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