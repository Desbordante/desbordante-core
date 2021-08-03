var Kafka = require('node-rdkafka');

var producer = new Kafka.Producer({
    //'debug' : 'all',
    'metadata.broker.list': 'localhost:9092',
    // 'dr_cb': true  
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

const client = Kafka.AdminClient.create({
  'client.id': 'kafka-admin',
  'bootstrap.servers' : 'localhost:9092',
  'metadata.broker.list': 'localhost:9092'
});

producer.connect()
  .on('ready', function(i, metadata) {
    // console.log(i);
    // console.log(metadata) 
    var is_topic_tasks_created = false
    metadata.topics.forEach((topic) => {
      if (topic.name === 'tasks') {
        is_topic_tasks_created = true;
        return;
      }
    })
    if (is_topic_tasks_created === true) {
      console.log(`Topic 'tasks' already exists`)
    } else {
      console.log(`Topic 'tasks' not found`)
      
      client.createTopic({
        topic: 'tasks',
        num_partitions: 1,
        replication_factor: 1
      }, function(res) {
        if (res !== undefined) console.log(res)
        else { 
          console.log(`Topic 'tasks' was created`)
          client.disconnect();
        }
      });
    }
  })
  .on('event.error', function(err) {
    console.log(err);
  })

module.exports = producer;