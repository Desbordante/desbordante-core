var express = require('express');
var router = express.Router();
const { v1: uuidv1 } = require('uuid');

router.post('/createTask', function(req, res){
    if(!req.body) return res.sendStatus(400);
  
    console.log(req.body);
    res.json({ status: "ok", taskID: uuidv1()  });
    
  }
);

module.exports = router;
