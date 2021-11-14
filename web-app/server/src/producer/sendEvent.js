const producer = require('./index');

async function sendEvent(topicName, taskID) {
    var value = Buffer.from(JSON.stringify({taskID}))
    var key = taskID
    // if partition is set to -1, librdkafka will use the default partitioner
    var partition = -1
    var headers = [
        { header: "header value" }
    ]
    await producer.produce(topicName, partition, value, key, Date.now(), "", headers)
}

module.exports = sendEvent;