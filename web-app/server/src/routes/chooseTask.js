var express = require('express');
var router = express.Router();

const { v1: uuidv1 } = require('uuid');

const eventTaskType = require('../kafka/eventTaskType');
var stream = require('../kafka/index');

// TODO (Use 'createTask.js')
// Choose tasks with server's dataset
router.post('/chooseTask', function(req, res){
    if(!req.body) return res.sendStatus(400);
    

    console.log(req.body);
    
    const dataset = 'server';
    const taskID = uuidv1()

    const event = { dataset, taskID };
    const success = stream.write(eventTaskType.toBuffer(event));

    if (success) {
        console.log(`message queued (${JSON.stringify(event)})`);
        res.json({ status: "OK", taskID: event.taskID });
    } else {
        console.log('Too many messages in the queue already..');
        res.json({ status: "ERROR", message: "Too many tasks in the queue already"})
    }
  }
);

module.exports = router;
