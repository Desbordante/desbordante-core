var express = require('express');
var router = express.Router();

router.get('/', function(req, res, next) {

    const allowedFileFormats = ["text/csv", "application/vnd.ms-excel"]
    const allowedAlgorithms = ["FastFDs", "Pyro", "TaneX"];
    const allowedSeparators = [",", "\\t", "\\n"];
    const maxFileSize = 50000000;
   
    res.send(JSON.stringify({ allowedFileFormats, allowedAlgorithms, allowedSeparators, maxFileSize }));
});

module.exports = router;
