var Kafka = require('node-rdkafka');

const stream = Kafka.Producer.createWriteStream({
    'metadata.broker.list': `${process.env.KAFKA_HOST}:${process.env.KAFKA_SERVER_PORT}`
}, {}, {
    topic: 'tasks'
});

stream.on('error', (error) => {
    console.error('Error in our kafka stream');
    console.error(error);
})

module.exports = stream;