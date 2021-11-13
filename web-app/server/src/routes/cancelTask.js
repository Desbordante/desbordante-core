var express = require('express');
var router = express.Router();

router.post('/cancelTask', function(req, res){
    if(!req.query || !req.query.taskID) return res.sendStatus(400);
    try {
        const pool = req.app.get('pool');
        req.query.taskID;
        const query = `update ${process.env.DB_TASKS_TABLE_NAME} set cancelled = true where taskID = '${req.query.taskID}'`;
        (
            async () => {
                await pool.query(query)
                .then(result => {
                    if (result !== undefined && result.rowCount == 1) {
                        res.send(result);
                        console.log(`Task with ID = '${req.query.taskID}' was cancelled`);
                    } else {
                        throw `Invalid taskID, task with ID = '${req.query.taskID}' wasn't cancelled`;
                    }
                })
                .catch(err => {
                    console.log(err);
                    res.status(400).send(err);
                })
            }
        ) ()
    } catch(err) {
        res.status(500).send('Unexpected problem caught: ' + err);
    }
});

module.exports = router;