#pragma once
#include <json.hpp>

#include "BaseConsumer.h"
#include "../db/TaskConfig.h"

class TaskConsumer : public BaseConsumer {
private:
    void processTask(TaskConfig const& task, DBManager const& manager) const;
    void processMsg(nlohmann::json payload, DBManager const &manager) const final;

public :
    explicit TaskConsumer(std::string brokers, std::string group_id,
                        std::string topic_name)
        : BaseConsumer(brokers, group_id, topic_name) {}

    ~TaskConsumer() override = default;
};