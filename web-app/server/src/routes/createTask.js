const express = require('express');
const { v1: uuidv1 } = require('uuid');
const router = express.Router();
var path = require('path');
const sendEvent = require('../producer/sendEvent');

router.post('/createTask', function(req, res){
    if(!req.body) return res.sendStatus(400)

    const pool = req.app.get('pool')
    const taskID = uuidv1()
    var table;
    let fileName;

    try {
        if(!req.files) {
            res.send({
                status: false,
                message: 'No file uploaded',
            });
        } else {
            table = req.files.file
            fileName = taskID + table.name
            // Use the mv() method to place the file in upload directory
            table.mv('./uploads/' + fileName)
        }
    } catch (err) {
        res.status(500).send('Problem with file downloading');
    }

    try {
        const json = JSON.parse(req.files.document.data)
        console.log("Input data:", json)
        console.log("File:", table)

        const { algName, errorPercent, separator, maxLHS, hasHeader } = json
        const status = 'ADDED TO THE TASK QUEUE'
        const progress = 0.0

        // get path to root file (www)
        var rootPath = path.dirname(require.main.filename).split("/")

        rootPath.pop()              // remove folder 'bin'
        rootPath.push('uploads')    // add folder 'uploads'
        rootPath.push(fileName)     // add file '${taskID} + filename.csv'

        const datasetPath = rootPath.join('/')
        
        var topicName = process.env.KAFKA_TOPIC_NAME;
        const query = `insert into ${process.env.DB_TASKS_TABLE_NAME}
            (taskID, createdAt, algName, errorPercent, separator, progress, 
            status, datasetPath, maxLHS, hasHeader, fileName) values
            ($1, now(), $2, $3, $4, $5, $6, $7, $8, $9, $10)`;
        const params = [taskID, algName, errorPercent, separator, progress, 
                        status, datasetPath, maxLHS, hasHeader, table.name];
    
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
                console.log("Record was added to kafka")
                let json = JSON.stringify({taskID, status: 'OK'})
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

module.exports = router;