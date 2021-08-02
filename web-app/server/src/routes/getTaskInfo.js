var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {
    if(!req.query) return res.sendStatus(400)

    try{
        // Get info about task from DB
        const pool = req.app.get('pool')
        pool.query(`select progress, status, fds::json, JSONArrayNameValue::json from tasks where taskid='${req.query.taskID}'`)
        .then(result => {
                if (result.rows[0] === undefined)
                    res.status(400).send("Invalid taskID");
                console.log(JSON.stringify(result.rows[0]));
                res.send(JSON.stringify(result.rows[0]));   
            }
        )
        .catch(err => {
                throw err
            }
        );
        

    } catch(err) {
        res.status(500).send(err);
    }

});

module.exports = router;
