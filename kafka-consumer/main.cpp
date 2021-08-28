#include "json.hpp"

#include "db/DBManager.h"

#include "consumer/consumerStart.h"
#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"

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
        auto brokers = "localhost:9092";
        auto topic_name = "tasks";
        auto group_id = "tasks_1";

        DBManager DBManager(pg_connection("pg_config.json", "desbordante"));
        // Create a ConsumerManager instance by passing properties
        ConsumerManager consumerManager(brokers, group_id);
        // Get consumer instance from object consumerManager
        auto const consumer = consumerManager.getConsumer();
        // Start consumer for topic with 'topic_name' name
        consumerStart(DBManager, consumer, topic_name);
    } catch (const std::exception& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
    }
}