var express = require('express');
var router = express.Router();
var path = require('path');

const { v1: uuidv1 } = require('uuid');

const eventTaskType = require('../producer/eventTaskType');
const producer = require('../producer/index');

// TODO: make one route (/createTask with param)
// Choose tasks with server's dataset (json must contain field fileName (*.csv) )
router.post('/chooseTask', function(req, res){
    if(!req.body) return res.sendStatus(400)

    const pool = req.app.get('pool')
    const taskID = uuidv1()
    
    try {
        const json = JSON.parse(req.files.document.data)

        console.log("Input data:", json)

        const { algName, errorPercent, semicolon, maxLHS, hasHeader, fileName } = json
        const status = 'NOT IN PROCESS'
        const progress = 0.0

        // get path to root file (www)
        var rootPath = path.dirname(require.main.filename).split("/")

        rootPath.pop()              // remove dir 'bin'
        rootPath.pop()              // remove dir 'server'
        rootPath.pop()              // remove dir 'web-app'
        rootPath.push('build')      // add dir 'build'
        rootPath.push('target')     // add dir 'target'
        rootPath.push('inputData')  // add dir 'inputData'
        rootPath.push(fileName)     // add file '*.csv'

        const datasetPath = rootPath.join('/')

        var topicName = 'tasks'
        const query = `insert into tasks(taskID, createdAt, algName, errorPercent, semicolon, progress, status, datasetPath, maxLHS, hasHeader) values\n
        ($1, now(), $2, $3, $4, $5, $6, $7, $8, $9)`;
        const params = [taskID, algName, errorPercent, semicolon, progress, status, datasetPath, maxLHS, hasHeader];
    
        // Add task to DB
        (async () => {
            await pool.query(query, params)
            .then(result => {
                if (result !== undefined) 
                    console.log(`Success (task [${taskID}] was added to DB)`)
                else
                    throw error('Problem with adding task to DB')
            })
            .catch(err => {
                console.log(`Error (task wasn't added to DB)`)
                res.status(400).send('Problem with adding task to DB')
            })
            await sendEvent(topicName,taskID)
            .then((result) => {
                let json = JSON.stringify({taskID, status: 'OK'})
                console.log("Record was added to kafka")
                console.log("Response: " + json)
                res.send(json)
            })
            .catch(err => {
                console.log(err)
                res.status(400).send('Problem with sending a response')
            })
        }) ()        
    } catch(err) {
        res.status(500).send('Unexpected problem caught: ' + err)
    }
});

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

module.exports = router;