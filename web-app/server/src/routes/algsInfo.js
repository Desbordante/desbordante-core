var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {

    const allowedAlgorithms = ["FastFDs", "Pyro", "TaneX"];
    const allowedSeparators = [",", "\\t", "\\n"];
    const maxFileSize = 50000000;
   
    res.send(JSON.stringify({ allowedAlgorithms, allowedSeparators, maxFileSize }));
});

module.exports = router;
