var express = require('express');
var router = express.Router();
var path = require('path');
const { v1: uuidv1 } = require('uuid');

const eventTaskType = require('../kafka-producer/eventTaskType');

// Choose tasks with server's dataset
router.post('/chooseTask', function(req, res){

    if(!req.body) return res.sendStatus(400)

    // TODO: add try catch

    const json = JSON.parse(req.files.document.data)

    const dataset = 'server'
    const taskID = uuidv1()
    const { algName, errorPercent, semicolon, fileName, maxLHS, hasHeader } = json
    const progress = 0.0

    // get path to root file (www)
    var rootPath = path.dirname(require.main.filename).split("/")

    rootPath.pop()              // remove 'bin'
    rootPath.push('target')     // add 'target'
    rootPath.push('inputData')  // add 'inputData'
    rootPath.push(fileName)     // add '*.csv'

    const datasetPath = rootPath.join('/')
    const status = 'NOT IN PROCESS'

    // Add task to DB
    const pool = req.app.get('pool')
    pool.query(
        `insert into tasks(taskID, createdAt, algName, errorPercent, semicolon, progress, status, datasetPath, maxLHS, hasHeader) values\n
        ($1, now(), $2, $3, $4, $5, $6, $7, $8, $9)`, [taskID, algName, errorPercent, semicolon, progress, status, datasetPath, maxLHS, hasHeader]
    )
    .then(res => {
            if (res !== undefined) 
                console.log(`Success (task was added to DB)`)
        }
    )
    .catch(err => {
            console.log(`Error (task wasn't added to DB)`)
            throw err
        }
    );

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
