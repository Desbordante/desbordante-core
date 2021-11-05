var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {
    if(!req.query || !req.query.taskID) return res.sendStatus(400);
    try{
        const pool = req.app.get('pool')
        var answer;
        pool.query(`select status, fileName from tasks where taskid = '${req.query.taskID}'`)
        .then(result => {
            if (result.rows[0] === undefined) {
                res.status(400).send("Invalid taskID");
                return;
            }
            const status = result.rows[0].status;
            if (status === 'SERVER ERROR') {
                answer = JSON.stringify(result.rows[0]);
                res.status(500).send(answer);
                return;
            }
            return status;
        })
        .then(status => {
            if (status === 'INCORRECT INPUT DATA') {
                pool.query(`select status, errorStatus, fileName from tasks where taskid = '${req.query.taskID}'`)
                .then(result => {
                    answer = JSON.stringify(result.rows[0]);
                    console.log(answer);
                    res.status(400).send(answer); 
                    return;
                })
                .catch(err => {
                    answer = 'SERVER ERROR: ' + err;
                    console.log(answer);
                    res.status(500).send(answer);
                    return;
                });
            } else if (status === 'COMPLETED') {
                pool.query(`select phaseName, progress, currentPhase, maxPhase, status, fileName, fds::json, arrayNameValue::json, columnNames::json, elapsedTime from tasks where taskid = '${req.query.taskID}'`)
                .then(result => { 
                    answer = JSON.stringify(result.rows[0]);
                    console.log(answer);
                    res.send(answer);
                    return;
                })
                .catch(err => {
                    answer = 'SERVER ERROR: ' + err;
                    console.log(answer);
                    res.status(500).send(answer);
                    return;
                });
            } else if (status === 'IN PROCESS' || status === 'ADDED TO THE TASK QUEUE') {
                console.log(status)
                pool.query(`select phaseName, progress, fileName, currentPhase, maxPhase, status from tasks where taskid = '${req.query.taskID}'`)
                .then(result => { 
                    answer = JSON.stringify(result.rows[0]);
                    console.log(answer);
                    res.send(answer);
                    return;
                })
                .catch(err => {
                    answer = 'SERVER ERROR: ' + err;
                    res.status(500).send(answer);
                    console.log(answer);
                    return;
                });
            }
        })
        .catch(err => {
            answer = 'SERVER ERROR: ' + err;
            console.log(answer);
            res.status(500).send(answer);
            return; 
        })
    } catch(err) {
        throw ('Unexpected server behavior [getTaskInfo]: ' + err);
    }
});

module.exports = router;
