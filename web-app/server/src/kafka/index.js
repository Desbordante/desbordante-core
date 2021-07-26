var Kafka = require('node-rdkafka');

const stream = Kafka.Producer.createWriteStream({
    'metadata.broker.list': 'localhost:9092'
}, {}, {
    topic: 'tasks'
});

stream.on('error', (error) => {
    console.error('Error in our kafka stream');
    console.error(error);
})

module.exports = stream;