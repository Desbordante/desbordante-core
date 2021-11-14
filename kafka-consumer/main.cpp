#include <fstream>

#include "json.hpp"

#include "db/DBManager.h"

#include "cppkafka/consumer.h"
#include "cppkafka/configuration.h"
#include "consumer/BaseConsumer.h"
#include "consumer/TaskConsumer.h"

static std::string dbConnection(std::string const &filename) {
    std::ifstream dbConfig(filename);
    nlohmann::json j;
    dbConfig >> j;

    std::string connection = "postgresql://";
    if(j.find("user") != j.end()){
        std::string user = j["user"];
        if(!user.empty()) connection += user;
    }
    if(j.find("password") != j.end()) {
        std::string password = j["password"];
        if(!password.empty()) connection += ":" + password;
    }
    if (j.find("host") != j.end()) {
        std::string host = j["host"];
        if (!host.empty()) connection += "@" + host;
    } else {
        connection += "@localhost";
    }
    if (j.find("port") != j.end()) {
        std::string port = j["port"];
        connection += ":" + port;
    }
    if (j.find("dbname") != j.end()) {
        std::string dbname = j["dbname"];
        connection += "/" + dbname;
        return connection;
    } else {
        throw std::runtime_error("Incorrect db_config.json");
    }
}

std::unique_ptr<BaseConsumer> taskConsumerInit(std::string const &filename) {
    std::ifstream consumerConfig(filename);
    nlohmann::json j;
    consumerConfig >> j;
    if (j.find("brokers") == j.end() || j.find("topic_name") == j.end()
        || j.find("group_id") == j.end()) {
        throw std::runtime_error("Incorrect kafka config");
    }
    return std::make_unique<TaskConsumer>(j["brokers"], j["group_id"], j["topic_name"]);
}

int main()
{
    try {
        DBManager DBManager(dbConnection("db_config.json"));
        auto taskConsumer = taskConsumerInit("task_consumer_config.json");
        taskConsumer->run(DBManager);
    } catch (const std::exception& e) {
        std::cerr << "% Unexpected exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}