var express = require('express');
var router = express.Router();
var path = require('path');

const { v1: uuidv1 } = require('uuid');
const sendEvent = require('../producer/sendEvent');

// Choose tasks with server's dataset (json must contain field fileName (*.csv) )
router.post('/chooseTask', function(req, res){
    if(!req.body) return res.sendStatus(400);

    const pool = req.app.get('pool');
    const taskID = uuidv1();
    
    try {
        const json = JSON.parse(req.files.document.data);

        console.log("Input data:", json);

        const { fileName } = req.body;
        const { algName, errorPercent, hasHeader, parallelism } = json;
        var maxLHS = 0;
        if (json.maxLHS) {
            maxLHS = json.maxLHS;
        }
        const separator = ',';
        const status = 'ADDED TO THE TASK QUEUE';
        const progress = 0.0;

        // get path to root file (www)
        var rootPath = path.dirname(require.main.filename).split("/");
        rootPath.pop();              // remove folder 'bin'
        rootPath.pop();              // remove folder 'server'
        rootPath.pop();              // remove folder 'web-app'
        rootPath.push('build');      // add folder 'build'
        rootPath.push('target');     // add folder 'target'
        rootPath.push('inputData');  // add folder 'inputData'
        rootPath.push(fileName);     // add file '*.csv'

        const datasetPath = rootPath.join('/');

        var topicName = process.env.KAFKA_TOPIC_NAME;
        const query = `insert into ${process.env.DB_TASKS_TABLE_NAME}
            (taskID, createdAt, algName, errorPercent, separator, progress, 
            status, datasetPath, maxLHS, hasHeader, fileName, parallelism, cancelled) values
            ($1, now(), $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, false)`;
        const params = [taskID, algName, errorPercent, separator, 
                        progress, status, datasetPath, maxLHS, 
                        hasHeader, fileName, parallelism];
    
        // Add task to DB
        (async () => {
            await pool.query(query, params)
            .then(result => {
                if (result !== undefined) {
                    console.log(`Success (task with ID = '${taskID}' was added to DB)`);
                } else {
                    throw error('Problem with adding task to DB');
                }
            })
            .catch(err => {
                res.status(400).send('Problem with adding task to DB');
                throw error('Problem with adding task to DB');
            })
            await sendEvent(topicName, taskID)
            .then((result) => {
                let json = JSON.stringify({ taskID, status: 'OK' });
                console.log("Record was added to kafka");
                console.log("Response: " + json);
                res.send(json);
            })
            .catch(err => {
                console.log(err);
                res.status(400).send('Problem with sending a response');
            })
        }) ()        
    } catch(err) {
        res.status(500).send('Unexpected problem caught: ' + err);
    }
});

module.exports = router;