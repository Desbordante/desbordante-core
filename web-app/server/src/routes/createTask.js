const express = require('express');
const { v1: uuidv1 } = require('uuid');
const router = express.Router();
const eventTaskType = require('../kafka/eventTaskType');
const stream = require('../kafka/index');

router.post('/createTask', function(req, res){
    if(!req.body) return res.sendStatus(400)
    try {
        if(!req.files) {
            res.send({
                status: false,
                message: 'No file uploaded',
            });
        } else {
            // Save file to ../uploads
            let table = req.files.file;
            
            // Use the mv() method to place the file in upload directory (i.e. "uploads")
            table.mv('./uploads/' + table.name);
        }
    } catch (err) {
        res.status(500).send(err);
    }

    // TODO: add try catch
    // console.log('Adding to DB')
    const json = JSON.parse(req.files.document.data);

    const dataset = 'user'
    const taskID = uuidv1()
    const { algName, errorPercent, semicolon } = json
    const progress = 0.0
    const fileName = req.files.file.name
    const datasetPath = '~/git_workspace/desbordante/build/target/inputData/' + fileName // TODO: FIX
    const status = 'NOT IN PROCESS'

    // Add task to DB
    const pool = req.app.get('pool')
    pool.query(
        `insert into tasks(taskID, createdAt, algName, errorPercent, semicolon, progress, status, datasetPath) values\n
        ($1, now(), $2, $3, $4, $5, $6, $7)`, [taskID, algName, errorPercent, semicolon, progress, status, datasetPath]
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

    // Add task to event (Kafka server)
    const event = { dataset, taskID };
    const success = stream.write(eventTaskType.toBuffer(event));

    if (success) {
        console.log(`message queued (${JSON.stringify(event)})`);
        res.json({ status: "OK", taskID: event.taskID });
    } else {
        console.log('Too many messages in the queue already..');
        res.json({ status: "ERROR", message: "Too many tasks in the queue already"})
    }
})

module.exports = router;