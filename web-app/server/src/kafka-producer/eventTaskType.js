const avro = require('avsc');

var eventTaskType = avro.Type.forSchema({
  type: 'record',
  fields: [
    {
      name: 'dataset',
      type: { type: 'enum', symbols: ['user', 'server'] }
    },
    {
      name: 'taskID',
      type: 'string',
    }
  ]
});

module.exports = eventTaskType;