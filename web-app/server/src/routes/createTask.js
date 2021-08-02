const express = require('express');
const { v1: uuidv1 } = require('uuid');
const router = express.Router();
const eventTaskType = require('../kafka-producer/eventTaskType');
const producer = require('../kafka-producer/index');
var path = require('path');

router.post('/createTask', function(req, res){
    if(!req.body) return res.sendStatus(400)

    const taskID = uuidv1()

    try {
        if(!req.files) {
            res.send({
                status: false,
                message: 'No file uploaded',
            });
        } else {
            // Save file to ../uploads
            let table = req.files.file

            // Use the mv() method to place the file in upload directory
            table.mv('./uploads/' + taskID + table.name)
        }
    } catch (err) {
        res.status(500).send(err);
    }

    // TODO: add try catch

    const json = JSON.parse(req.files.document.data)
    console.log(json)

    const dataset = 'user'
    const { algName, errorPercent, semicolon, maxLHS, hasHeader } = json
    const progress = 0.0
    const fileName = req.files.file.name

    // get path to root file (www)
    var rootPath = path.dirname(require.main.filename).split("/")

    rootPath.pop()                       // remove 'bin'
    rootPath.push('uploads')             // add 'uploads'
    rootPath.push(taskID + fileName)     // add '*.csv' + unique key

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
    var sendMessage = async () => {
        var topicName = 'tasks';
        var value = Buffer.from(JSON.stringify(event));
        // TODO (for what we can use key)
        var key = "key";
        // if partition is set to -1, librdkafka will use the default partitioner
        var partition = -1;
        var headers = [
        { header: "header value" }
        ]
        await producer.produce(topicName, partition, value, key, Date.now(), "", headers)
    }

    sendMessage()
    .then((result) => {
        res.send(JSON.stringify({taskID, status: 'OK'}));   
    })
    .catch(err => {
        res.sendStatus(400)
    });


});

module.exports = router;