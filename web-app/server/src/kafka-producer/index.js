var Kafka = require('node-rdkafka');

var producer = new Kafka.Producer({
    //'debug' : 'all',
    'metadata.broker.list': 'localhost:9092',
    // 'dr_cb': true  
    //delivery report callback
  }, {}, {
        topic: 'tasks'
});

producer.on('event.log', function(log) {
    console.log(log);
});

producer.on('event.error', function(err) {
    console.error('Error from producer');
    console.error(err);
});

producer.on('disconnected', function(arg) {
    console.log('producer disconnected. ' + JSON.stringify(arg));
  });
  
// starting the producer
producer.connect();

module.exports = producer;