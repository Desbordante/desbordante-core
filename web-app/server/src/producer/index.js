var Kafka = require('node-rdkafka');

var producer = new Kafka.Producer(
  { 'metadata.broker.list': `${process.env.KAFKA_HOST}:${process.env.KAFKA_SERVER_PORT}` }, 
  {}, 
  { topic: `${process.env.TOPIC}` }
);

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
  'client.id':            `${process.env.KAFKA_ADMIN_CLIENT_ID}`,
  'bootstrap.servers' :   `${process.env.KAFKA_HOST}:${process.env.KAFKA_SERVER_PORT}`,
  'metadata.broker.list': `${process.env.KAFKA_HOST}:${process.env.KAFKA_SERVER_PORT}`
});

producer.connect()
  .on('ready', function(i, metadata) {
    var is_topic_tasks_created = false
    metadata.topics.forEach((topic) => {
      if (topic.name === `${process.env.DB_TASKS_TABLE_NAME}`) {
        is_topic_tasks_created = true;
        return;
      }
    })
    if (is_topic_tasks_created === true) {
      console.log(`Topic '${process.env.DB_TASKS_TABLE_NAME}' already exists`)
    } else {
      console.log(`Topic '${process.env.DB_TASKS_TABLE_NAME}' not found`)
      
      client.createTopic({
        topic: `${process.env.DB_TASKS_TABLE_NAME}`,
        num_partitions: 1,
        replication_factor: 1
      }, function(res) {
        if (res !== undefined) console.log(res)
        else { 
          console.log(`Topic '${process.env.DB_TASKS_TABLE_NAME}' was created`)
          client.disconnect();
        }
      });
    }
  })
  .on('event.error', function(err) {
    console.log(err);
  })

module.exports = producer;